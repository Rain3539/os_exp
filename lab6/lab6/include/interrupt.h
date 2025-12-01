// interrupt.h - 中断处理框架头文件
#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"
#include "proc.h"

// 中断处理函数类型
typedef void (*interrupt_handler_t)(struct trapframe *);

// 中断优先级定义
#define IRQ_PRIORITY_HIGHEST   0
#define IRQ_PRIORITY_TIMER     1
#define IRQ_PRIORITY_EXTERNAL  2
#define IRQ_PRIORITY_SOFTWARE  3
#define IRQ_PRIORITY_LOWEST    4

// 中断号定义
#define IRQ_TIMER      5
#define IRQ_EXTERNAL   9
#define IRQ_SOFTWARE   1

// 最大中断数
#define MAX_INTERRUPTS 256

// 中断管理函数
void trap_init(void);
void register_interrupt(int irq, interrupt_handler_t handler);
void enable_interrupt(int irq);
void disable_interrupt(int irq);
interrupt_handler_t get_interrupt_handler(int irq);
int can_nest_interrupts(void);
void enter_interrupt(void);
void exit_interrupt(void);

// 中断控制器
void interrupt_controller_init(void);
void interrupt_controller_enable(int irq);
void interrupt_controller_disable(int irq);


// 异常处理
void register_exception_handlers(void);
void handle_exception(struct trapframe *tf);

// 时钟和调度
void yield(void);
uint64 get_time(void);
void sbi_set_timer(uint64 time);
void set_next_timer_interrupt(void);

// 测试函数
void test_interrupt_system(void);
void test_timer_interrupt(void);
void test_exception_handling(void);
void test_interrupt_overhead(void);
void test_interrupt_nesting(void);
void interrupt_demo(void);  // 中断演示函数

#endif /* INTERRUPT_H */
