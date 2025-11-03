// 主启动文件
// 包含系统初始化、测试函数和启动流程

#include "def.h"
#include "type.h"
#include "utils/console.h"
#include "utils/string.h"
#include "mm/memlayout.h"
#include "mm/riscv.h"
#include "fs/fs.h"
#include "fs/bio.h"
#include "fs/log.h"
#include "fs/file.h"
#include "fs/syscall_fs.h"
#include "proc/proc.h"

// 外部变量和函数声明
extern struct superblock sb;
extern int simulate_crash_after_commit;
extern pagetable_t create_pagetable(void);
extern void free_pagetable(pagetable_t pagetable);
extern void *kalloc(void);
extern void kfree(void *pa);
extern uint64 get_time(void);

// =================================================================
// 测试1: 文件系统完整性测试
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

// =================================================================
// 测试2: 并发访问测试
// =================================================================

// 子进程任务
void child_task(void) {
    struct proc *p = myproc();
    char filename[32];
    
    // 使用 PID 构造唯一文件名
    snprintf(filename, sizeof(filename), "test_%d", p->pid);
    
    // 每个子进程循环10次，快速执行create-write-close-unlink
    for (int j = 0; j < 10; j++) {
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

// 并发访问测试主函数
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

// =================================================================
// 测试3: 崩溃恢复测试
// =================================================================
void test_crash_recovery(void) {
    printf("--- 3. Test: Crash Recovery (Replay) ---\n");
    char buf[64];
    const char *path = "crashfile";
    const char *data_A = "Initial-Data-A";
    const char *data_B = "Crash-Data-B";
    int fd, r;

    // 步骤1: 创建一个文件并写入 "Data-A"
    printf("  1. Setup: Creating '%s' with 'Data-A'\n", path);
    fd = open(path, O_CREATE | O_RDWR);
    if(fd < 0) panic("crash test setup open failed");
    if(write(fd, data_A, strlen(data_A)) != strlen(data_A))
        panic("crash test setup write failed");
    close(fd);

    // 步骤2: 模拟崩溃
    printf("  2. Crash: Writing 'Data-B' with O_TRUNC...\n");
    
    simulate_crash_after_commit = 1; // 激活崩溃模拟标志
    
    fd = open(path, O_RDWR | O_TRUNC);
    if(fd < 0) panic("crash test open 2 failed");
    
    r = write(fd, data_B, strlen(data_B));
    
    // close() 会触发 end_op() -> commit()
    // commit() 将看到标志, 写入日志和头, 然后返回 (模拟崩溃)
    close(fd);
    
    simulate_crash_after_commit = 0; // 关闭标志

    // 步骤3: 模拟重启
    printf("  3. Reboot: Simulating system reboot...\n");
    
    // 模拟 RAM 丢失 (清除块缓存)
    binit();
    
    // 模拟内核启动 (重载日志系统)
    initlog(1, &sb);

    // 步骤4: 验证重放
    printf("  4. Verify: Checking for 'Data-B'\n");
    fd = open(path, O_RDONLY);
    if(fd < 0) panic("crash test verify open failed");
    
    r = read(fd, buf, sizeof(buf)-1);
    buf[r] = 0;
    close(fd);
    
    // 验证文件内容
    if(strcmp(buf, data_B) == 0) {
        printf("  [PASS] 'Data-B' was successfully recovered!\n");
    } else {
        printf("  [FAIL] Read '%s', expected '%s'\n", buf, data_B);
        panic("Crash recovery test failed");
    }
    
    unlink(path);
}

// =================================================================
// 测试4: 性能测试
// =================================================================
void test_filesystem_performance(void) {
    printf("--- 4. Test: Filesystem Performance ---\n");
    uint64 start_time;
    
    // 大量小文件测试 - 创建100个小文件，每个文件写入4字节数据（元数据密集型操作）
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

    // 大文件测试 - 创建一个大文件，写入1MB数据（数据密集型操作）
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
// 内核第一个进程 - init
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
// 内核主函数
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