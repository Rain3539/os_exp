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

//printf.c
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

//uart.c
void uart_putc(char c);

//console.c
void cons_putc(int c);
void cons_puts(const char *s);

// kalloc.c
void*           kalloc(void);
void            kfree(void *);
void            kinit(void);

// string.c
void* memset(void *dst, int c, uint n);


// vm.c
void            kvminit(void);
void            kvminithart(void);
void            kvmmap(pagetable_t, uint64, uint64, uint64, int);

int             mappages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t     create_pagetable(void);
void            free_pagetable(pagetable_t);
pte_t *         walk(pagetable_t, uint64, int);
uint64          walkaddr(pagetable_t, uint64);
int             ismapped(pagetable_t, uint64);

// trap.c
void trapinithart(void);
void test_timer_interrupt(void);
void test_breakpoint(void);
void test_syscall(void);
void test_exception(void);

// 全局变量外部声明
extern volatile int global_interrupt_count;

// RISC-V异常和中断类型定义
#define CAUSE_INTERRUPT_FLAG    0x8000000000000000L

// 异常类型 (scause & ~CAUSE_INTERRUPT_FLAG)
#define CAUSE_INSTRUCTION_MISALIGNED    0 // 指令地址未对齐
#define CAUSE_INSTRUCTION_ACCESS_FAULT  1 // 指令访问故障
#define CAUSE_ILLEGAL_INSTRUCTION       2 // 非法指令
#define CAUSE_BREAKPOINT               3 // 断点指令
#define CAUSE_LOAD_MISALIGNED          4 // 加载地址未对齐
#define CAUSE_LOAD_ACCESS_FAULT        5 // 加载访问故障
#define CAUSE_STORE_MISALIGNED         6 // 存储地址未对齐
#define CAUSE_STORE_ACCESS_FAULT       7 // 存储访问故障
#define CAUSE_USER_ECALL               8 // 用户模式系统调用
#define CAUSE_SUPERVISOR_ECALL         9 // 特权模式系统调用
#define CAUSE_MACHINE_ECALL            11 // 机器模式系统调用
#define CAUSE_INSTRUCTION_PAGE_FAULT   12 // 指令页故障
#define CAUSE_LOAD_PAGE_FAULT          13 // 加载页故障
#define CAUSE_STORE_PAGE_FAULT         15 // 存储页故障

// 中断类型 (scause & ~CAUSE_INTERRUPT_FLAG)
#define CAUSE_SOFTWARE_INTERRUPT       1 // 软件中断
#define CAUSE_TIMER_INTERRUPT          5 // 定时器中断
#define CAUSE_EXTERNAL_INTERRUPT       9 // 外部中断

// 系统调用号定义
#define SYS_EXIT    1
#define SYS_GETPID  2
#define SYS_FORK    3
#define SYS_WAIT    4
#define SYS_READ    5
#define SYS_WRITE   6
#define SYS_OPEN    7
#define SYS_CLOSE   8
#define SYS_EXEC    9
#define SYS_SBRK    10

// 陷阱帧结构体定义
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
    /* 248 */ uint64 epc;    // 单独处理，不在栈上
};

// 异常处理函数声明
void handle_exception(struct trapframe *tf);
void handle_syscall(struct trapframe *tf);
void handle_instruction_page_fault(struct trapframe *tf);
void handle_load_page_fault(struct trapframe *tf);
void handle_store_page_fault(struct trapframe *tf);

// syscall.c
uint64 sys_exit(void);
uint64 sys_getpid(void);
uint64 sys_fork(void);
uint64 sys_wait(void);
uint64 sys_read(void);
uint64 sys_write(void);
uint64 sys_open(void);
uint64 sys_close(void);
uint64 sys_exec(void);
uint64 sys_sbrk(void);
