// 进程管理相关结构体和常量定义
#ifndef PROC_H
#define PROC_H
#include "../type.h"
#include "../mm/riscv.h"

// 最大进程数
#define NPROC 64

// 进程状态枚举
enum procstate { 
  UNUSED,    // 未使用
  USED,      // 已分配但未初始化完成
  SLEEPING,  // 睡眠中
  RUNNABLE,  // 可运行
  RUNNING,   // 正在运行
  ZOMBIE     // 僵尸进程
};

// 上下文结构体 - 保存调度时需要的寄存器
struct context {
  uint64 ra;   // 返回地址
  uint64 sp;   // 栈指针
  
  // 被调用者保存寄存器(callee-saved)
  uint64 s0;
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

// CPU结构体
struct cpu {
  struct proc *proc;          // 当前运行的进程
  struct context context;     // CPU调度器上下文
  int noff;                   // 中断关闭嵌套层数
  int intena;                 // 在push_off()之前中断是否开启
};

// 进程结构体
struct proc {
  enum procstate state;        // 进程状态
  int pid;                     // 进程ID
  int priority;                // 优先级(0-4,数字越小优先级越高)
  pagetable_t pagetable;       // 用户页表
  struct trapframe *trapframe; // 陷阱帧指针
  struct context context;      // 进程调度上下文
  uint64 kstack;              // 内核栈虚拟地址
  uint64 sz;                  // 进程内存大小(字节)
  void *chan;                 // 睡眠通道
  int killed;                 // 是否被杀死
  int xstate;                 // 退出状态
  struct proc *parent;        // 父进程指针
  char name[16];              // 进程名称(用于调试)
  void (*entry_func)(void);   // 进程入口函数指针
};

extern struct cpu cpus[1];
extern struct proc proc[NPROC];

// 函数声明
struct cpu* mycpu(void);
struct proc* myproc(void);
void procinit(void);
int create_process(void (*entry)(void), char *name, int priority);
void yield(void);
void sched(void);
void scheduler(void);
void sleep(void *chan);
void wakeup(void *chan);
void exit(int status);
int wait(int *status);
int kill(int pid);
void debug_proc_table(void);
void freeproc(struct proc *p);
void push_off(void);
void pop_off(void);

// 汇编函数声明
void swtch(struct context *old, struct context *new);

#endif // PROC_H