// sysfile.c - 文件相关系统调用实现

#include "kernel.h"
#include "syscall.h"
#include "proc.h"
#include "defs.h"
#include "printf.h"
#include "file.h"
#include "param.h"
#include "console.h"

#define CONSOLE 1  // 控制台设备号

extern struct proc proc[NPROC];
extern struct proc* myproc(void);
extern struct devsw devsw[];
extern int consolewrite(int user_src, uint64 src, int n);
extern int consoleread(int user_dst, uint64 dst, int n);

// O_RDONLY等标志（在使用前定义）
#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200
#define O_TRUNC   0x400

// 字符串比较辅助函数（在使用前定义）
static int strncmp(const char *s1, const char *s2, int n)
{
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i])
            return s1[i] - s2[i];
        if (s1[i] == 0)
            return 0;
    }
    return 0;
}

// 辅助函数：获取文件描述符
static struct file* getfd(struct proc *p, int fd)
{
    if (fd < 0 || fd >= NOFILE)
        return 0;
    return p->ofile[fd];
}

// sys_open - 打开文件（简化实现：只支持控制台）
int sys_open(void)
{
    char path[512];
    int omode;
    struct proc *p = myproc();
    struct file *f;

    if (argstr(0, path, sizeof(path)) < 0 || argint(1, &omode) < 0)
        return -1;

    // 简化实现：只支持打开控制台
    // 检查路径是否为"/dev/console"或类似
    if (strncmp(path, "/dev/console", 12) == 0 || 
        strncmp(path, "console", 7) == 0 ||
        strncmp(path, "/console", 8) == 0) {
        // 创建文件结构（简化：静态分配）
        static struct file console_file;
        f = &console_file;
        f->type = FD_DEVICE;
        f->major = CONSOLE;
        f->ref = 1;
        f->readable = (omode & O_RDONLY) != 0 || (omode & O_RDWR) != 0;
        f->writable = (omode & O_WRONLY) != 0 || (omode & O_RDWR) != 0;
        f->ip = 0;
        f->pipe = 0;
        f->off = 0;
        
        // 分配文件描述符
        for (int fd = 0; fd < NOFILE; fd++) {
            if (p->ofile[fd] == 0) {
                p->ofile[fd] = f;
                return fd;
            }
        }
        return -1;  // 无可用文件描述符
    }

    // 其他文件暂不支持
    printf("[sys_open] 不支持打开文件: %s\n", path);
    return -1;
}

// sys_close - 关闭文件
int sys_close(void)
{
    int fd;
    struct proc *p = myproc();
    struct file *f;

    if (argint(0, &fd) < 0)
        return -1;
    
    f = getfd(p, fd);
    if (f == 0)
        return -1;
    
    p->ofile[fd] = 0;
    // 简化实现：不处理引用计数
    return 0;
}

// sys_read - 从文件读取数据
int sys_read(void)
{
    int fd;
    int n;
    uint64 p;

    if (argint(0, &fd) < 0 || argaddr(1, &p) < 0 || argint(2, &n) < 0)
        return -1;
    
    struct proc *proc = myproc();
    struct file *f = getfd(proc, fd);
    
    if (f == 0)
        return -1;

    int is_user = proc && proc->pagetable;

    if (f->type == FD_DEVICE) {
        if (f->major < NDEV && devsw[f->major].read) {
            return devsw[f->major].read(is_user, p, n);
        }
    }

    return -1;
}

// sys_write - 向文件写入数据
int sys_write(void)
{
    struct file *f;
    int n;
    uint64 p;

    int fd;
    if (argint(0, &fd) < 0 || argaddr(1, &p) < 0 || argint(2, &n) < 0)
        return -1;
    
    struct proc *proc = myproc();
    f = getfd(proc, fd);
    if (f == 0)
        return -1;

    int is_user = proc && proc->pagetable;

    if (f->type == FD_DEVICE) {
        if (f->major < NDEV && devsw[f->major].write) {
            return devsw[f->major].write(is_user, p, n);
        }
    }

    return -1;
}

// sys_pipe - 创建管道（简化实现：返回错误）
int sys_pipe(void)
{
    printf("[sys_pipe] pipe系统调用暂未实现\n");
    return -1;
}

// sys_exec - 执行程序（简化实现：返回错误）
int sys_exec(void)
{
    printf("[sys_exec] exec系统调用暂未实现\n");
    return -1;
}

// sys_fstat - 获取文件状态（简化实现：返回错误）
int sys_fstat(void)
{
    printf("[sys_fstat] fstat系统调用暂未实现\n");
    return -1;
}

// sys_chdir - 切换目录（简化实现：返回错误）
int sys_chdir(void)
{
    printf("[sys_chdir] chdir系统调用暂未实现\n");
    return -1;
}

// sys_dup - 复制文件描述符（简化实现：返回错误）
int sys_dup(void)
{
    printf("[sys_dup] dup系统调用暂未实现\n");
    return -1;
}

// sys_mknod - 创建设备文件（简化实现：返回错误）
int sys_mknod(void)
{
    printf("[sys_mknod] mknod系统调用暂未实现\n");
    return -1;
}

// sys_unlink - 删除文件（简化实现：返回错误）
int sys_unlink(void)
{
    printf("[sys_unlink] unlink系统调用暂未实现\n");
    return -1;
}

// sys_link - 创建硬链接（简化实现：返回错误）
int sys_link(void)
{
    printf("[sys_link] link系统调用暂未实现\n");
    return -1;
}

// sys_mkdir - 创建目录（简化实现：返回错误）
int sys_mkdir(void)
{
    printf("[sys_mkdir] mkdir系统调用暂未实现\n");
    return -1;
}

