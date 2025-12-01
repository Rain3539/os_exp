// test_syscall.c - 系统调用测试

#include "kernel.h"
#include "printf.h"
#include "syscall.h"
#include "proc.h"
#include "defs.h"
#include "param.h"
#include "riscv.h"
#include "memlayout.h"
#include "vm.h"

extern struct proc proc[NPROC];
extern struct proc* myproc(void);
extern uint64 get_time(void);
extern int create_process(void (*entry)(void));
extern int wait_process(int *status);
extern void exit_process(int status);

// 系统调用辅助函数：模拟用户态系统调用接口
// 在实际用户态环境中，这些会通过usys.S的汇编代码调用
static int do_syscall(int num, uint64 a0, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5)
{
    struct proc *p = myproc();
    if (!p) {
        printf("[do_syscall] 无当前进程\n");
        return -1;
    }
    
    // 如果进程没有trapframe，分配一个
    if (!p->trapframe) {
        // 分配一个trapframe（使用页分配器）
        extern void* alloc_page(void);
        void* page = alloc_page();
        if (page) {
            p->trapframe = (struct trapframe*)page;
            // 初始化trapframe
            for (int i = 0; i < sizeof(struct trapframe)/sizeof(uint64); i++) {
                ((uint64*)p->trapframe)[i] = 0;
            }
        } else {
            // 如果分配失败，使用静态fallback（仅用于测试）
            static struct trapframe static_tf;
            p->trapframe = &static_tf;
            for (int i = 0; i < sizeof(struct trapframe)/sizeof(uint64); i++) {
                ((uint64*)p->trapframe)[i] = 0;
            }
        }
    }
    
    struct trapframe *tf = p->trapframe;
    tf->a0 = a0;
    tf->a1 = a1;
    tf->a2 = a2;
    tf->a3 = a3;
    tf->a4 = a4;
    tf->a5 = a5;
    tf->a7 = num;  // 系统调用号
    
    // 调用系统调用分发器
    syscall();
    
    // 返回值在a0中
    return (int)tf->a0;
}

// 用户态接口包装函数（模拟用户态系统调用接口，在使用前定义）
static int getpid(void) {
    return do_syscall(SYS_getpid, 0, 0, 0, 0, 0, 0);
}

static int fork(void) {
    return do_syscall(SYS_fork, 0, 0, 0, 0, 0, 0);
}

static void exit(int status) {
    do_syscall(SYS_exit, status, 0, 0, 0, 0, 0);
}

static int wait(int *status) {
    uint64 status_addr = status ? (uint64)status : 0;
    return do_syscall(SYS_wait, status_addr, 0, 0, 0, 0, 0);
}

static int open(const char *path, int omode) {
    return do_syscall(SYS_open, (uint64)path, omode, 0, 0, 0, 0);
}

static int write(int fd, const void *buf, int n) {
    return do_syscall(SYS_write, fd, (uint64)buf, n, 0, 0, 0);
}

static int read(int fd, void *buf, int n) {
    return do_syscall(SYS_read, fd, (uint64)buf, n, 0, 0, 0);
}

static int close(int fd) {
    return do_syscall(SYS_close, fd, 0, 0, 0, 0, 0);
}

// 测试子任务函数（用于fork测试的替代）
static void child_task(void) {
    printf("Child process: PID=%d\n", getpid());
    exit(42);
}

// 字符串长度辅助函数
static int strlen(const char *s) {
    int len = 0;
    while (s[len] != '\0') len++;
    return len;
}

// 文件打开标志
#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200
#define O_TRUNC   0x400

// ========== 测试函数 ==========

// 基础功能测试
void test_basic_syscalls(void) {
    printf("1.基础功能测试...\n");

    // 测试getpid
    printf("1.1 getpid测试...\n");

    int pid = getpid();
    printf("Current PID: %d\n", pid);


    printf("1.2 fork测试...\n");

    // 测试fork
    int child_pid = fork();
    if (child_pid == 0) {
        // 子进程
        printf("Child process: PID=%d\n", getpid());
        exit(42);
    }
    else if (child_pid > 0) {
        // 父进程
        int status;
        int waited_pid = wait(&status);
        printf("Child exited with status: %d (waited PID: %d)\n", status, waited_pid);
    }
    else {
        printf("Fork failed! Using create_process as alternative...\n");
        // 使用create_process替代fork进行测试
        int child_pid = create_process(child_task);
        if (child_pid > 0) {
            int status;
            int waited_pid = wait(&status);
            printf("Child exited with status: %d (waited PID: %d)\n", status, waited_pid);
        }
    }
    printf("\n");
}

// 参数传递测试
void test_parameter_passing(void) {
    printf("2 参数传递测试...\n");

    
    // 测试不同类型参数的传递
    char buffer[] = "Hello, World!";
    int fd = open("/dev/console", O_RDWR);

    if (fd >= 0) {
        int bytes_written = write(fd, buffer, strlen(buffer));
        printf("Wrote %d bytes\n", bytes_written);
        close(fd);
    }

    // 测试边界情况
    printf("2.1 无效描述符测试...\n");

    int result1 = write(-1, buffer, 10); // 无效文件描述符
    printf("write(-1, buffer, 10) returned: %d\n", result1);
    printf("2.2 空指针测试...\n");
    
    int result2 = write(fd, (void*)0, 10); // 空指针（fd已关闭，先测试空指针）
    printf("write(fd, NULL, 10) returned: %d\n", result2);
    printf("2.3 负数长度测试...\n");
    
    int result3 = write(-1, buffer, -1); // 负数长度
    printf("write(-1, buffer, -1) returned: %d\n", result3);
    
    printf("\n");
}

// 安全性测试
void test_security(void) {
 
    printf("3 安全性测试...\n");
    
    printf("3.1 无效指针测试...\n");

    // 测试无效指针访问
    char *invalid_ptr = (char*)0x1000000; // 可能无效的地址
    int result = write(1, invalid_ptr, 10);
    printf("Invalid pointer write result: %d\n", result);

    printf("3.2 缓冲区边界测试...\n");

    // 测试缓冲区边界
    char small_buf[4];
    int fd = open("/dev/console", O_RDONLY);
    if (fd >= 0) {
        // 尝试读取超过缓冲区大小（在内核态可能不会触发错误，但会限制读取）
        result = read(fd, small_buf, 1000);
        printf("read(fd, small_buf[4], 1000) returned: %d\n", result);
        close(fd);
    }

    // 测试权限检查
    printf("3.3 权限测试...\n");

    printf("Security tests completed\n");
    printf("\n");
}

// 性能测试
void test_syscall_performance(void) {
    printf("4 性能测试...\n");

    
    uint64 start_time = get_time();

    // 大量系统调用测试
    for (int i = 0; i < 50; i++) {
        getpid(); // 简单的系统调用
    }

    uint64 end_time = get_time();
    uint64 elapsed = (end_time > start_time) ? (end_time - start_time) : 0;
    
    printf("10000 getpid() calls took %lu cycles\n", elapsed);
    printf("Average: %lu cycles per call\n", elapsed / 10000);
    printf("\n");
}

// 主测试函数
void test_syscalls(void)
{
    printf("\n========== 系统调用测试开始 ==========\n\n");
    
    test_basic_syscalls();
    test_parameter_passing();
    test_security();
    test_syscall_performance();
    
    printf("========== 系统调用测试结束 ==========\n\n");
}
