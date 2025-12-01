/**
 * 内核定义头文件
 * 包含内核中使用的函数声明和常量
 */

#ifndef DEFS_H
#define DEFS_H

#include "types.h"
#include "param.h"

// UART 函数
void uart_init(void);      // UART初始化
void uart_putc(char c);    // UART发送字符
void uart_puts(char *s);   // UART发送字符串
void uartputc_sync(int c); // 同步UART发送字符

// 控制台函数  
void consputc(int c);           // 控制台输出字符
void console_puts(const char* s); // 控制台输出字符串
void clear_screen(void);        // 清屏
void consoleinit(void);         // 控制台初始化
void consoleintr(int c);        // 控制台中断处理

// 扩展控制台功能
void goto_xy(int x, int y);     // 光标定位
void clear_line(void);          // 清除当前行
int printf_color(int color, char* fmt, ...); // 颜色输出

// 打印函数
int printf(char* fmt, ...);  // 格式化输出
void panic(char* s);         // 内核恐慌处理
void printfinit(void);       // 打印系统初始化

// 进程函数
void procdump(void); // 添加 procdump 声明

// 进程管理接口（实验5）
struct proc* alloc_process(void);
void free_process(struct proc* p);
int create_process(void (*entry)(void));
void exit_process(int status);
int wait_process(int *status);

// 前向声明
struct spinlock;
struct proc;
struct cpu;
struct context;

// 自旋锁函数
void initlock(struct spinlock* lk, char* name); // 初始化自旋锁
void acquire(struct spinlock* lk);              // 获取自旋锁
void release(struct spinlock* lk);              // 释放自旋锁

// 进程函数
struct proc* myproc(void);     // 获取当前进程
int killed(struct proc* p);    // 检查进程是否被杀死
void kexit(int status);        // 进程退出
void setkilled(struct proc* p); // 标记进程为被杀死
void sched(void);              // 进程切换回调度器
void scheduler_step(void);     // 手动触发调度
extern volatile int need_resched; // 调度请求标志
void swtch(struct context*, struct context*); // 上下文切换汇编

// 睡眠/唤醒函数
void sleep(void* chan, struct spinlock* lk); // 进程睡眠
void wakeup(void* chan);                     // 唤醒进程

// 拷贝函数
int either_copyin(void* dst, int user_src, uint64 src, uint64 len);  // 从用户/内核空间拷贝数据
int either_copyout(int user_dst, uint64 dst, void* src, uint64 len); // 拷贝数据到用户/内核空间

// 中断函数
void push_off(void);  // 禁用中断
void pop_off(void);   // 恢复中断
void intr_on(void);   // 启用中断
void intr_off(void);  // 禁用中断
int intr_get(void);   // 获取中断状态

// 系统调用
void syscall(void);   // 系统调用处理

// 页错误处理
int vmfault(pagetable_t pagetable, uint64 stval, int is_load); // 虚拟内存错误处理

// 时钟中断
void clockintr(void); // 时钟中断处理

// PLIC中断处理
int plic_claim(void); // 声明PLIC中断
void plic_complete(int irq); // 完成PLIC中断处理

// 设备中断
void uartintr(void); // UART中断处理
void virtio_disk_intr(void); // 虚拟磁盘中断处理

// 陷阱处理
void prepare_return(void); // 准备返回用户空间

// CPU相关
int cpuid(void); // 获取CPU ID
struct cpu* mycpu(void); // 获取当前CPU

// 常量定义
#define UART0 0x10000000L  // UART0基地址
#define CONSOLE 1          // 控制台设备号

// 中断号定义
#define UART0_IRQ 10       // UART中断号
#define VIRTIO0_IRQ 1      // VIRTIO中断号

// 设备开关表
struct devsw {
    int (*read)(int, uint64, int);   // 设备读函数指针
    int (*write)(int, uint64, int);  // 设备写函数指针
};

extern struct devsw devsw[];  // 外部设备表声明

void test_interrupt_system_safe(void);  // 安全测试函数

#endif /* DEFS_H */
