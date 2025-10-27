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

// 进程入口包装函数
static void
proc_entry(void)
{
  struct proc *p = myproc();
  
  // 执行进程的实际入口函数
  if(p->entry_func) {
    p->entry_func();
  } else {
    // 理论上不应该发生
    panic("proc_entry: no entry function");
  }
  
  // 进程函数返回后自动退出
  exit(0);
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
  p->entry_func = 0;
  
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
  // p->kstack = KSTACK((int) (p - proc));

  // 为内核栈分配一个物理页
  if((p->kstack = (uint64)kalloc()) == 0) {
    freeproc(p);
    return 0;
  }

  // 清空上下文
  memset(&p->context, 0, sizeof(p->context));
  
  // 设置上下文:返回地址指向proc_entry包装函数
  p->context.ra = (uint64)proc_entry;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

// 释放进程资源
void
freeproc(struct proc *p)
{
  //1.释放trapframe
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  //2.释放页表
  if(p->pagetable)
    free_pagetable(p->pagetable);
  p->pagetable = 0;

  // 释放内核栈
  if(p->kstack)
      kfree((void*)p->kstack);
  p->kstack = 0;
  
  //3.清空字段
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->entry_func = 0;
  //4.设置状态为UNUSED
  p->state = UNUSED;
}

// 初始化进程系统
void
procinit(void)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    p->state = UNUSED;
    p->kstack = 0; // KSTACK宏不再需要，在allocproc中实际分配
  }
  
  // 初始化CPU的context为0
  memset(&cpus[0].context, 0, sizeof(struct context));
  cpus[0].noff = 0;
  cpus[0].intena = 0;
}

// 创建一个新进程
// entry: 进程入口函数
// name: 进程名称
// priority: 进程优先级
int
create_process(void (*entry)(void), char *name, int priority)
{
  //1.分配进程
  struct proc *p;
  
  p = allocproc();
  if(p == 0) {
    return -1;
  }

  // 2.设置进程名称
  int i;
  for(i = 0; i < 15 && name[i]; i++) {
    p->name[i] = name[i];
  }
  p->name[i] = 0;

  // 3.设置优先级
  p->priority = priority;

  // 4.保存入口函数
  p->entry_func = entry;
  
  // 5.设置为可运行状态
  p->state = RUNNABLE;
  
  //6.返回进程ID
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
  
  if(!p)
    panic("sched: no proc");
    
  if(p->state == RUNNING)
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
  
  printf("Scheduler started, looking for runnable processes...\r\n");
  
  int count = 0;
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
      
      // --- 新增：在这里打印进程表，展示状态变化 ---
      debug_proc_table(); 

    } else {
      // 没有可运行的进程
      if(count % 100000000 == 0) {
        printf("No runnable processes\r\n");
      }
      count++;
    }
  }
}


//进程同步相关函数实现
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
  if(p->parent)
    wakeup(p->parent);

  // 将子进程转交给init进程
  struct proc *pp;
  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      if(initproc)
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

// 进程状态调试
void debug_proc_table(void) {
  // 定义状态字符串数组
  // 确保这些枚举值 (UNUSED, SLEEPING 等) 在 proc.h 中定义
  static char *states[] = {
      [UNUSED]   "UNUSED",
      [USED]     "USED",
      [SLEEPING] "SLEEPING",
      [RUNNABLE] "RUNNABLE",
      [RUNNING]  "RUNNING",
      [ZOMBIE]   "ZOMBIE"
  };
  
  printf("=== Process Table ===\n");
  printf("PID\tPriority\tState\t\tName\n"); // 添加表头
  printf("--------------------------------------------\n"); // 添加分隔符

  for (int i = 0; i < NPROC; i++) {
      struct proc *p = &proc[i];
      if (p->state != UNUSED) {
          
          // 确保状态在范围内，防止数组越界
          char *state_str = "UNKNOWN";
          if(p->state >= UNUSED && p->state <= ZOMBIE) {
              state_str = states[p->state];
          }

          // 修改 printf，加入 p->priority
          printf("%d\t%d\t\t%s\t\t%s\n",
                 p->pid, p->priority, state_str, p->name);
      }
  }
  printf("============================================\n"); // 结束符
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