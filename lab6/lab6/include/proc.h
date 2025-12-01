/**
 * 进程头文件
 * 包含进程相关定义
 */

#ifndef PROC_H
#define PROC_H

#include "types.h"
#include "param.h"
#include "file.h"
#include "spinlock.h"

// 前向声明
struct context;
struct trapframe;

// 本实验使用的简化类型
typedef uint64 pagetable_t;  // 页表类型

// 进程状态枚举
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// 简化上下文结构体
struct context {
    uint64 ra;   // 返回地址
    uint64 sp;   // 栈指针
    uint64 s0;   // 保存寄存器s0-s11
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};

// 简化陷阱帧结构体
struct trapframe {
    uint64 kernel_satp;   // 内核页表寄存器
    uint64 kernel_sp;     // 内核栈指针
    uint64 kernel_trap;   // 内核陷阱处理函数
    uint64 epc;           // 异常程序计数器
    uint64 kernel_hartid; // 内核硬件线程ID
    uint64 ra;            // 返回地址
    uint64 sp;            // 栈指针
    uint64 gp;            // 全局指针
    uint64 tp;            // 线程指针
    uint64 t0;            // 临时寄存器t0-t6
    uint64 t1;
    uint64 t2;
    uint64 s0;            // 保存寄存器s0-s11
    uint64 s1;
    uint64 a0;            // 参数寄存器a0-a7
    uint64 a1;
    uint64 a2;
    uint64 a3;
    uint64 a4;
    uint64 a5;
    uint64 a6;
    uint64 a7;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
    uint64 t3;
    uint64 t4;
    uint64 t5;
    uint64 t6;
};

// 每个CPU的状态
struct cpu {
    struct proc *proc;      // 在此CPU上运行的进程，或为空
    struct context context; // 切换到这里进入调度器
    int noff;               // push_off()嵌套深度
    int intena;             // push_off()前中断是否启用
};

extern struct cpu cpus[NCPU];  // 外部CPU数组声明

// 每个进程的状态
struct proc {
    struct spinlock lock;   // 进程锁

    // 使用这些字段时必须持有p->lock:
    enum procstate state;   // 进程状态
    void *chan;             // 如果不为零，表示在chan上睡眠
    int killed;             // 如果不为零，表示已被杀死
    int xstate;             // 返回给父进程wait的退出状态
    int pid;                // 进程ID

    // 使用此字段时必须持有wait_lock:
    struct proc *parent;    // 父进程

    // 这些是进程私有的，不需要持有p->lock:
    uint64 kstack;          // 内核栈虚拟地址
    uint64 sz;              // 进程内存大小(字节)
    pagetable_t pagetable;  // 用户页表
    struct trapframe *trapframe; // trampoline.S的数据页
    struct context context;      // 切换到这里运行进程
    struct file *ofile[NOFILE];  // 打开的文件
    struct inode *cwd;           // 当前目录
    char name[16];               // 进程名(调试用)
    void (*entry)(void);         // 进程入口（内核任务函数）
};

extern struct proc proc[NPROC];    // 进程表（由 simplified.c 提供）

#endif /* PROC_H */