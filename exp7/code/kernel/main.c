#include "def.h"
#include "type.h"
#include "utils/console.h"
#include "mm/memlayout.h"     // for PGSIZE
#include "mm/riscv.h"         // for r_time
#include "fs/fs.h"
#include "fs/bio.h"
#include "fs/log.h"
#include "fs/file.h"
#include "proc/proc.h"        // 包含进程管理

// 外部超级块
extern struct superblock sb;

// 外部函数
extern int simulate_crash_after_commit; // 访问 log.c 中的标志
extern pagetable_t create_pagetable(void);
extern void free_pagetable(pagetable_t pagetable);
extern void *kalloc(void);
extern void kfree(void *pa);

// =================================================================
// 缺失的工具函数 (在内核中实现)
// =================================================================
#define NULL 0
#define assert(x) if (!(x)) panic("assertion failed")

int strlen(const char *s) {
    int n = 0;
    while(s[n])
        n++;
    return n;
}

int strcmp(const char *p, const char *q) {
    while(*p && *p == *q)
        p++, q++;
    return (uchar)*p - (uchar)*q;
}

// -----------------------------------------------------------------
// 修复: 更健壮的 snprintf (支持一个 %d)
// -----------------------------------------------------------------
// 简化的 itoa (整数到字符串)
static void itoa(int n, char *buf) {
    char temp[16];
    int i = 0;
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    // 处理负数
    int is_neg = n < 0;
    if (is_neg) n = -n;

    // 倒序填充
    while (n > 0) {
        temp[i++] = (n % 10) + '0';
        n /= 10;
    }
    if (is_neg) temp[i++] = '-';
    
    // 倒序写回 buf
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

// 简化的 snprintf，可正确处理一个 %d
void snprintf(char *buf, int size, const char *fmt, ...) {
    int *va = (int*)(&fmt + 1); // 获取第一个参数
    int num_arg = *va;
    char num_buf[16];          // 存放 %d 转换后的字符串
    
    char *p = buf;
    const char *f = fmt;
    
    while(*f && (p - buf) < size - 1) {
        if (*f == '%') {
            f++;
            if (*f == 'd') {
                // 找到了 %d, 转换数字
                itoa(num_arg, num_buf);
                char *n_ptr = num_buf;
                while (*n_ptr && (p - buf) < size - 1) {
                    *p++ = *n_ptr++;
                }
            } else {
                // 不支持的格式 (例如 %s), 只复制 '%'
                *p++ = '%';
            }
        } else {
            // 复制普通字符
            *p++ = *f;
        }
        f++;
    }
    *p = '\0'; // 字符串结尾
}
// -----------------------------------------------------------------


// =================================================================
// 缺失的系统调用层实现
// =================================================================

// 分配文件描述符
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *p = myproc();

  for(fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd] == 0){
      p->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

// 查找 inode
// (这是 create 的辅助函数)
static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;

  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && ip->type == T_FILE)
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // 创建 . 和 ..
    dp->nlink++;  // .. 指向父目录
    iupdate(dp);
    // 链接 .
    if(dirlink(ip, ".", ip->inum) < 0)
      panic("create dots");
    // 链接 ..
    if(dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);
  return ip;
}

// open (实现了 O_CREATE, O_RDWR, O_RDONLY, O_WRONLY, O_TRUNC)
int open(const char *path, int omode)
{
  int fd;
  struct file *f;
  struct inode *ip;

  begin_op();

  if(omode & O_CREATE){
    ip = create((char*)path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei((char*)path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  // O_TRUNC (截断) 逻辑
  if(omode & O_TRUNC){
    if(ip->type != T_FILE)
        panic("O_TRUNC on non-file");
    
    // itrunc 会清空文件大小和数据块
    itrunc(ip); 
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0; // 截断或新文件，偏移量都为0
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  iunlock(ip);
  end_op();
  return fd;
}

// read
int read(int fd, void *buf, int n)
{
  struct file *f;
  struct proc *p = myproc();
  if(fd < 0 || fd >= NOFILE || (f=p->ofile[fd]) == 0)
    return -1;
  
  return fileread(f, (uint64)buf, n);
}

// write
int write(int fd, const void *buf, int n)
{
  struct file *f;
  struct proc *p = myproc();
  if(fd < 0 || fd >= NOFILE || (f=p->ofile[fd]) == 0)
    return -1;

  return filewrite(f, (uint64)buf, n);
}

// close
int close(int fd)
{
  struct file *f;
  struct proc *p = myproc();
  if(fd < 0 || fd >= NOFILE || (f=p->ofile[fd]) == 0)
    return -1;

  p->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

// unlink (fs.c 中缺失的实现)
int unlink(const char *path)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ];
  uint off;

  begin_op();
  if((dp = nameiparent((char*)path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);

  // 查找目录项
  if(dirlookup(dp, name, &off) == 0){
    iunlockput(dp);
    end_op();
    return -1;
  }

  // 获取 inode
  if((ip = dirlookup(dp, name, 0)) == 0)
    panic("unlink: dirlookup failed");
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR){
    iunlockput(ip);
    iunlockput(dp);
    end_op();
    return -1; // 不允许 unlink 目录
  }

  // 擦除目录项
  memset(&de, 0, sizeof(de));
  if(writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  
  iunlockput(dp);

  // 减少链接数
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();
  return 0;
}

// =================================================================
// 文件系统测试套件
// =================================================================

void test_filesystem_integrity(void) {
    printf("--- 1. Test: Filesystem Integrity ---\n");
    
    // 创建测试文件
    int fd = open("testfile", O_CREATE | O_RDWR);
    assert(fd >= 0);
    
    // 写入数据
    char buffer[] = "Hello, filesystem!";
    int bytes = write(fd, buffer, strlen(buffer));
    assert(bytes == strlen(buffer));
    close(fd);
    
    // 重新打开并验证
    fd = open("testfile", O_RDONLY);
    assert(fd >= 0);
    char read_buffer[64];
    bytes = read(fd, read_buffer, sizeof(read_buffer) - 1);
    read_buffer[bytes] = '\0';
    assert(strcmp(buffer, read_buffer) == 0);
    close(fd);
    
    // 删除文件
    assert(unlink("testfile") == 0);
    
    printf("  [PASS] Filesystem integrity test passed.\n");
}

// test_concurrent_access 的子进程任务
void child_task(void) {
    struct proc *p = myproc();
    char filename[32];
    
    // 使用 PID 构造唯一文件名
    snprintf(filename, sizeof(filename), "test_%d", p->pid);
    
    for (int j = 0; j < 10; j++) { // (减少循环次数以加快测试)
        int fd = open(filename, O_CREATE | O_RDWR);
        if (fd >= 0) {
            write(fd, &j, sizeof(j));
            close(fd);
            unlink(filename);
        }
    }
    printf("  [Child %d] Task finished.\n", p->pid);
    exit(0);
}

// 并发访问测试 (已修改为使用 create_process)
void test_concurrent_access(void) {
    printf("--- 2. Test: Concurrent Access ---\n");
    
    int num_children = 4;
    
    // 创建多个进程同时访问文件系统
    for (int i = 0; i < num_children; i++) {
        char name[16];
        snprintf(name, sizeof(name), "child%d", i);
        
        int pid = create_process(child_task, name, 2);
        if (pid > 0) {
            printf("  [Init] Created child process %d (%s)\n", pid, name);
        } else {
            printf("  [Init] Failed to create child %d\n", i);
        }
    }
    
    // 等待所有子进程完成
    for (int i = 0; i < num_children; i++) {
        int status;
        int pid = wait(&status);
        printf("  [Init] Waited for child %d to exit.\n", pid);
    }
    
    printf("  [PASS] Concurrent access test completed.\n");
}

// 崩溃恢复测试 (模拟重放)
void test_crash_recovery(void) {
    printf("--- 3. Test: Crash Recovery (Replay) ---\n");
    char buf[64];
    const char *path = "crashfile";
    const char *data_A = "Initial-Data-A";
    const char *data_B = "Crash-Data-B";
    int fd, r;

    // --- 1. Setup: 创建一个文件并写入 "Data-A" ---
    printf("  1. Setup: Creating '%s' with 'Data-A'\n", path);
    fd = open(path, O_CREATE | O_RDWR);
    if(fd < 0) panic("crash test setup open failed");
    if(write(fd, data_A, strlen(data_A)) != strlen(data_A))
        panic("crash test setup write failed");
    close(fd);

    // --- 2. 模拟崩溃 ---
    printf("  2. Crash: Writing 'Data-B' with O_TRUNC...\n");
    
    simulate_crash_after_commit = 1; // 激活崩溃模拟标志
    
    fd = open(path, O_RDWR | O_TRUNC); // 使用 O_TRUNC
    if(fd < 0) panic("crash test open 2 failed");
    
    r = write(fd, data_B, strlen(data_B));
    
    // close() 会触发 end_op() -> commit()
    // commit() 将看到标志, 写入日志和头, 然后返回 (模拟崩溃)
    close(fd);
    
    simulate_crash_after_commit = 0; // 关闭标志

    // --- 3. 模拟重启 ---
    printf("  3. Reboot: Simulating system reboot...\n");
    
    // 模拟 RAM 丢失 (清除块缓存)
    binit();
    
    // 模拟内核启动 (重载日志系统)
    initlog(1, &sb);

    // --- 4. 验证重放 ---
    printf("  4. Verify: Checking for 'Data-B'\n");
    fd = open(path, O_RDONLY);
    if(fd < 0) panic("crash test verify open failed");
    
    r = read(fd, buf, sizeof(buf)-1);
    buf[r] = 0;
    close(fd);

    if(strcmp(buf, data_B) == 0) {
        printf("  [PASS] 'Data-B' was successfully recovered!\n");
    } else {
        printf("  [FAIL] Read '%s', expected '%s'\n", buf, data_B);
        panic("Crash recovery test failed");
    }
    
    unlink(path);
}

// 性能测试
void test_filesystem_performance(void) {
    printf("--- 4. Test: Filesystem Performance ---\n");
    uint64 start_time;
    
    // 大量小文件测试
    int n_small_files = 100;
    printf("  Task: Creating %d small files...\n", n_small_files);
    
    start_time = get_time();
    for (int i = 0; i < n_small_files; i++) {
        char filename[32];
        snprintf(filename, sizeof(filename), "small_%d", i);

        int fd = open(filename, O_CREATE | O_RDWR);
        write(fd, "test", 4);
        close(fd);
    }
    uint64 small_files_time = get_time() - start_time;

    // 大文件测试
    printf("  Task: Creating 1MB large file...\n");
    start_time = get_time();
    int fd = open("large_file", O_CREATE | O_RDWR);
    char large_buffer[1024]; // 1KB buffer
    for (int i = 0; i < 1024; i++) { // 1MB文件
        write(fd, large_buffer, sizeof(large_buffer));
    }
    close(fd);
    uint64 large_file_time = get_time() - start_time;
    
    printf("  Result: Small files (100x4B): %d cycles\n", small_files_time);
    printf("  Result: Large file (1x1MB): %d cycles\n", large_file_time);

    // 清理测试文件
    printf("  Task: Cleaning up test files...\n");
    for (int i = 0; i < n_small_files; i++) {
        char filename[32];
        snprintf(filename, sizeof(filename), "small_%d", i);
        unlink(filename);
    }
    unlink("large_file");
    
    printf("  [PASS] Performance test finished.\n");
}


// =================================================================
// 内核的第一个进程
// =================================================================
void init_main(void) {
    printf("\n[Init Process Started] Running test suite...\n\n");
    
    // 运行所有测试
    test_filesystem_integrity();    printf("\n");
    test_concurrent_access();       printf("\n");
    test_crash_recovery();          printf("\n");
    test_filesystem_performance();  printf("\n");
    
    printf("[All Tests Completed] System halting.\n");
    
    // 停止系统
    while(1);
}

// =================================================================
// 主函数
// =================================================================
void main(void) {
    printf("My RISC-V OS Starting...\r\n");
    printf("Lab 7+: File System + Processes\r\n\r\n");

    // 初始化各个子系统
    printf("===== [Boot] System Initialization =====\r\n");
    
    printf("  Initializing memory management...\r\n");
    kinit();
    
    printf("  Initializing virtual memory...\r\n");
    kvminit();
    kvminithart();
    
    printf("  Initializing trap handling...\r\n");
    trapinithart();
    
    printf("\r\n  Initializing block cache...\r\n");
    binit();
    
    printf("  Initializing file table...\r\n");
    fileinit();
    
    printf("  Initializing file system (dev 1)...\r\n");
    fsinit(1);
    
    printf("\r\n  Initializing process table...\r\n");
    procinit();
    
    printf("===== [Boot] Initialization Complete =====\r\n\r\n");

    // 创建第一个进程 (init)
    printf("[Boot] Creating init process...\r\n");
    if(create_process(init_main, "init", 1) < 0) {
        panic("main: failed to create init process");
    }

    // 启动调度器
    printf("[Boot] Starting scheduler...\r\n");
    scheduler();
    
    // scheduler 永远不应该返回
    panic("scheduler returned");
}