#ifndef DEF_H
#define DEF_H

#include "type.h"
#include "mm/riscv.h"

// 自定义assert宏
#define assert(condition) \
    do { \
        if (!(condition)) { \
            printf("Assertion failed: %s, file %s, line %d\n", \
                   #condition, __FILE__, __LINE__); \
            panic("Assertion failed"); \
        } \
    } while(0)

// ========== 控制台和输出函数 ==========
void printint(long long value, int base, int sgn);
void printf(const char *fmt, ...);
void panic(char *s);
void clear_screen(void);
void clear_line(void);
void goto_xy(int x, int y);
void set_text_color(const char *color);
void set_background_color(const char *color);
void reset_colors(void);
void printf_color(const char *color, const char *fmt, ...);
void uart_putc(char c);
void cons_putc(int c);
void cons_puts(const char *s);

// ========== 内存管理函数 ==========
void* kalloc(void);
void  kfree(void *);
void  kinit(void);
void* memset(void *dst, int c, uint n);

// ========== 虚拟内存管理函数 ==========
void        kvminit(void);
void        kvminithart(void);
void        kvmmap(pagetable_t, uint64, uint64, uint64, int);
int         mappages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t create_pagetable(void);
void        free_pagetable(pagetable_t);
pte_t*      walk(pagetable_t, uint64, int);
uint64      walkaddr(pagetable_t, uint64);
int         ismapped(pagetable_t, uint64);

// ========== 陷阱处理函数 ==========
void trapinithart(void);
void test_timer_interrupt(void);
void test_breakpoint(void);
void test_syscall(void);
void test_exception(void);

// 全局变量外部声明
extern volatile int global_interrupt_count;

// ========== RISC-V异常和中断类型定义 ==========
#define CAUSE_INTERRUPT_FLAG    0x8000000000000000L

// 异常类型
#define CAUSE_INSTRUCTION_MISALIGNED    0
#define CAUSE_INSTRUCTION_ACCESS_FAULT  1
#define CAUSE_ILLEGAL_INSTRUCTION       2
#define CAUSE_BREAKPOINT               3
#define CAUSE_LOAD_MISALIGNED          4
#define CAUSE_LOAD_ACCESS_FAULT        5
#define CAUSE_STORE_MISALIGNED         6
#define CAUSE_STORE_ACCESS_FAULT       7
#define CAUSE_USER_ECALL               8
#define CAUSE_SUPERVISOR_ECALL         9
#define CAUSE_MACHINE_ECALL            11
#define CAUSE_INSTRUCTION_PAGE_FAULT   12
#define CAUSE_LOAD_PAGE_FAULT          13
#define CAUSE_STORE_PAGE_FAULT         15

// 中断类型
#define CAUSE_SOFTWARE_INTERRUPT       1
#define CAUSE_TIMER_INTERRUPT          5
#define CAUSE_EXTERNAL_INTERRUPT       9

// ========== 陷阱帧结构体定义 ==========
struct trapframe {
     /*   0 */ uint64 ra;
    /*   8 */ uint64 sp;
    /*  16 */ uint64 gp;
    /*  24 */ uint64 tp;
    /*  32 */ uint64 t0;
    /*  40 */ uint64 t1;
    /*  48 */ uint64 t2;
    /*  56 */ uint64 s0;
    /*  64 */ uint64 s1;
    /*  72 */ uint64 a0;
    /*  80 */ uint64 a1;
    /*  88 */ uint64 a2;
    /*  96 */ uint64 a3;
    /* 104 */ uint64 a4;
    /* 112 */ uint64 a5;
    /* 120 */ uint64 a6;
    /* 128 */ uint64 a7;
    /* 136 */ uint64 s2;
    /* 144 */ uint64 s3;
    /* 152 */ uint64 s4;
    /* 160 */ uint64 s5;
    /* 168 */ uint64 s6;
    /* 176 */ uint64 s7;
    /* 184 */ uint64 s8;
    /* 192 */ uint64 s9;
    /* 200 */ uint64 s10;
    /* 208 */ uint64 s11;
    /* 216 */ uint64 t3;
    /* 224 */ uint64 t4;
    /* 232 */ uint64 t5;
    /* 240 */ uint64 t6;
    /* 248 */ uint64 epc;
};

// ========== 异常处理函数声明 ==========
void handle_exception(struct trapframe *tf);
void handle_syscall(struct trapframe *tf);
void handle_instruction_page_fault(struct trapframe *tf);
void handle_load_page_fault(struct trapframe *tf);
void handle_store_page_fault(struct trapframe *tf);

// ========== 进程管理函数（来自proc.c） ==========
struct proc;
struct cpu;
struct context;

void         procinit(void);
struct cpu*  mycpu(void);
struct proc* myproc(void);
int          create_process(void (*entry)(void), char *name, int priority);
void         yield(void);
void         sched(void);
void         scheduler(void) __attribute__((noreturn));
void         forkret(void);
void         sleep(void *chan);
void         wakeup(void *chan);
void         exit(int status) __attribute__((noreturn));
int          wait(int *status);
int          kill(int pid);
void         debug_proc_table(void);
void         push_off(void);
void         pop_off(void);
void         swtch(struct context*, struct context*);

// ========== 文件系统函数（来自fs/）==========
struct file;

void         fileinit(void);
void         fileclose(struct file *f);

#endif // DEF_H