
/**
 * RISC-V OS主入口点
 * 此文件包含汇编初始化后运行的C主函数
 */

#include "uart.h"
#include "kernel.h"
#include "printf.h"
#include "console.h"
#include "vm.h"
#include "riscv.h"

// 外部函数声明
extern void kernelvec();
extern void trap_init(void);
extern void run_proc_tests(void);
extern void debug_proc_table(void);
extern void test_syscalls(void);
extern int create_process(void (*entry)(void));
extern int wait_process(int *status);
extern void exit_process(int status);

// 调度接口
int register_task(void (*fn)(void));
void scheduler_step(void);
extern volatile int need_resched;

// 系统调用测试任务
static void syscall_test_task(void) {
    printf("开始执行系统调用测试套件...\n\n");
    test_syscalls();
    printf("系统调用测试完成\n\n");
    exit_process(0);
}

/**
 * 从汇编入口点调用的主内核函数
 * 执行硬件初始化并运行主内核循环
 */
void start() {
    // 初始化UART用于串行通信
    uart_init();
    
    // 初始化printf系统
    printfinit();
    
    // 初始化控制台系统
    consoleinit();
    
    printf("\n=== RISC-V OS 启动 ===\n");
    
    // 初始化陷阱/中断处理
    trap_init();
     
    // 创建一个测试进程来运行系统调用测试
    // 因为系统调用需要在进程上下文中运行
    int test_pid = create_process(syscall_test_task);
    if (test_pid > 0) {
        printf("创建测试进程 PID: %d\n", test_pid);
        // 等待测试进程完成（在主线程中，使用循环等待）
        int status;
        for (;;) {
            int waited_pid = wait_process(&status);
            if (waited_pid == test_pid || waited_pid < 0) {
                break;
            }
            // 运行调度器让测试进程执行
            extern void scheduler_step(void);
            scheduler_step();
        }
    } else {
        printf("无法创建测试进程，直接运行测试...\n\n");
        test_syscalls();
    }

    // 最终状态 - 系统空闲循环
    printf("进入系统空闲循环...\n");
    while (1) {

    }
}
