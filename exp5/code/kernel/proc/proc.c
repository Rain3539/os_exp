// 进程管理核心实现
#include "proc.h"
#include "../def.h"
#include "../mm/memlayout.h"

// 全局进程表
struct proc proc[NPROC];

// CPU结构（单核）
struct cpu cpus[1];

// 下一个要分配的进程ID
static int nextpid = 1;

// 初始进程（idle进程）
struct proc *initproc;

// 获取当前CPU
struct cpu*
mycpu(void) 
{
  return &cpus[0];
}

// 获取当前进程
struct proc*
myproc(void) 
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

// 分配一个进程结构体
// 成功返回进程指针，失败返回0
static struct proc*
allocproc(void)
{
  struct proc *p;

  // 在进程表中查找UNUSED状态的进程槽位
  for(p = proc; p < &proc[NPROC]; p++) {
    if(p->state == UNUSED) {
      goto found;
    }
  }
  return 0;

found:
  p->pid = nextpid++;
  p->state = USED;
  p->priority = 2;  // 默认优先级为中等
  
  // 分配陷阱帧
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    return 0;
  }

  // 分配用户页表
  if((p->pagetable = create_pagetable()) == 0){
    freeproc(p);
    return 0;
  }

  // 设置内核栈
  p->kstack = KSTACK((int) (p - proc));

  // 清空上下文
  memset(&p->context, 0, sizeof(p->context));
  
  // 设置上下文的返回地址为forkret
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

// 释放进程资源
void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  
  if(p->pagetable)
    free_pagetable(p->pagetable);
  p->pagetable = 0;
  
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// 初始化进程系统
void
procinit(void)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    p->state = UNUSED;
    p->kstack = KSTACK((int)(p - proc));
  }
}

// 第一个用户进程初始化后的返回函数
void
forkret(void)
{
  // 第一次调度到这个进程时会来到这里
  // 可以做一些进程初始化工作
  
  // 返回到用户空间（如果有的话）
  // usertrapret();
}

// 创建一个新进程
// entry: 进程入口函数
// name: 进程名称
// priority: 进程优先级
int
create_process(void (*entry)(void), char *name, int priority)
{
  struct proc *p;
  
  p = allocproc();
  if(p == 0) {
    return -1;
  }

  // 设置进程名称
  int i;
  for(i = 0; i < 15 && name[i]; i++) {
    p->name[i] = name[i];
  }
  p->name[i] = 0;

  // 设置优先级
  p->priority = priority;

  // 设置陷阱帧，使进程从entry开始执行
  p->trapframe->epc = (uint64)entry;
  p->trapframe->sp = p->kstack + PGSIZE;  // 内核栈

  p->state = RUNNABLE;
  
  return p->pid;
}

// 进程主动让出CPU
void
yield(void)
{
  struct proc *p = myproc();
  
  if(p) {
    p->state = RUNNABLE;
  }
  sched();
}

// 切换到调度器
void
sched(void)
{
  struct proc *p = myproc();
  struct cpu *c = mycpu();
  
  if(p && p->state == RUNNING)
    panic("sched running");
  
  int intena = intr_get();
  swtch(&p->context, &c->context);
  
  // 恢复中断状态
  if(intena) {
    intr_on();
  }
}

// 调度器 - 优先级调度算法
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  
  for(;;){
    // 开启中断，允许设备中断
    intr_on();
    
    // 优先级调度：选择优先级最高（数字最小）的RUNNABLE进程
    struct proc *best = 0;
    int best_priority = 100;  // 初始化为一个很大的值
    
    for(p = proc; p < &proc[NPROC]; p++) {
      if(p->state == RUNNABLE) {
        if(p->priority < best_priority) {
          best_priority = p->priority;
          best = p;
        }
      }
    }
    
    // 如果找到可运行的进程，切换过去
    if(best) {
      p = best;
      p->state = RUNNING;
      c->proc = p;
      
      swtch(&c->context, &p->context);
      
      // 进程切换回来
      c->proc = 0;
    }
  }
}

// 进程睡眠（等待条件）
void
sleep(void *chan)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // 被唤醒后清空通道
  p->chan = 0;
}

// 唤醒等待在chan上的所有进程
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc() && p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
    }
  }
}

// 进程退出
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // 关闭所有打开的文件（这里简化，不实现文件系统）

  // 唤醒父进程
  wakeup(p->parent);

  // 将子进程转交给init进程
  struct proc *pp;
  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }

  // 进入僵尸状态
  p->xstate = status;
  p->state = ZOMBIE;

  // 跳转到调度器
  sched();
  panic("zombie exit");
}

// 等待子进程退出
int
wait(int *status)
{
  struct proc *p = myproc();
  struct proc *pp;
  int havekids;
  int pid;

  for(;;){
    // 扫描进程表找子进程
    havekids = 0;
    for(pp = proc; pp < &proc[NPROC]; pp++){
      if(pp->parent == p){
        havekids = 1;
        if(pp->state == ZOMBIE){
          // 找到退出的子进程
          pid = pp->pid;
          if(status != 0)
            *status = pp->xstate;
          freeproc(pp);
          return pid;
        }
      }
    }

    // 没有子进程
    if(!havekids){
      return -1;
    }

    // 等待子进程退出
    sleep(p);
  }
}

// 杀死进程
int
kill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // 唤醒进程，让它退出
        p->state = RUNNABLE;
      }
      return 0;
    }
  }
  return -1;
}

// 打印进程表（调试用）
void
debug_proc_table(void)
{
  struct proc *p;
  static char *states[] = {
    [UNUSED]    "unused",
    [USED]      "used",
    [SLEEPING]  "sleep",
    [RUNNABLE]  "runnable",
    [RUNNING]   "run",
    [ZOMBIE]    "zombie"
  };

  printf("\n=== Process Table ===\n");
  printf("PID\tPriority\tState\t\tName\n");
  
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state != UNUSED){
      printf("%d\t%d\t\t%s\t\t%s\n", 
             p->pid, p->priority, states[p->state], p->name);
    }
  }
  printf("====================\n\n");
}

// 关闭中断（嵌套）
void
push_off(void)
{
  int old = intr_get();
  
  intr_off();
  struct cpu *c = &cpus[0];
  if(c->noff == 0)
    c->intena = old;
  c->noff += 1;
}

// 恢复中断（嵌套）
void
pop_off(void)
{
  struct cpu *c = &cpus[0];
  
  if(intr_get())
    panic("pop_off - interruptible");
  if(c->noff < 1)
    panic("pop_off");
  
  c->noff -= 1;
  if(c->noff == 0 && c->intena)
    intr_on();
}