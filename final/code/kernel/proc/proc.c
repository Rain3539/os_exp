// 进程管理核心实现 - 优先级调度版本
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

static int last_selected_idx = -1;

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
    panic("proc_entry: no entry function");
  }
  
  // 进程函数返回后自动退出
  exit(0);
}

// 分配一个进程结构体
struct proc*
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
  p->priority = DEFAULT_PRIORITY;  // 设置默认优先级
  p->ticks = 0;                    // 初始化CPU时间
  p->wait_time = 0;                // 初始化等待时间
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
  
  memset(p->ofile, 0, sizeof(p->ofile));
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

  if(p->kstack)
      kfree((void*)p->kstack);
  p->kstack = 0;
  
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->entry_func = 0;
  p->priority = DEFAULT_PRIORITY;
  p->ticks = 0;
  p->wait_time = 0;
  p->state = UNUSED;
}

// 初始化进程系统
void
procinit(void)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    p->state = UNUSED;
    p->kstack = 0;
    p->priority = DEFAULT_PRIORITY;
    p->ticks = 0;
    p->wait_time = 0;
  }
  
  memset(&cpus[0].context, 0, sizeof(struct context));
  cpus[0].noff = 0;
  cpus[0].intena = 0;
  
  printf("进程系统初始化完成 (优先级调度)\n");
  printf("优先级范围: %d-%d, 默认优先级: %d\n", 
         MIN_PRIORITY, MAX_PRIORITY, DEFAULT_PRIORITY);
}

// 创建一个新进程
int
create_process(void (*entry)(void), char *name, int priority)
{
  struct proc *p;
  
  p = allocproc();
  if(p == 0) {
    return -1;
  }
  
  p->parent = myproc();
  
  // 设置进程名称
  int i;
  for(i = 0; i < 15 && name[i]; i++) {
    p->name[i] = name[i];
  }
  p->name[i] = 0;

  // 设置优先级（检查范围）
  if(priority < MIN_PRIORITY) priority = MIN_PRIORITY;
  if(priority > MAX_PRIORITY) priority = MAX_PRIORITY;
  p->priority = priority;

  // 保存入口函数
  p->entry_func = entry;
  
  // 设置为可运行状态
  p->state = RUNNABLE;
  
  return p->pid;
}

// 选择最高优先级的可运行进程
struct proc*
select_highest_priority(void)
{
  struct proc *p, *best = 0;
  int best_priority = MIN_PRIORITY - 1;
  int found = 0;
  
  // 从上次选择的位置之后开始搜索
  int start_idx = (last_selected_idx + 1) % NPROC;
  
  for(int i = 0; i < NPROC; i++) {
    p = &proc[(start_idx + i) % NPROC];
    if(p->state == RUNNABLE) {
      if(p->priority > best_priority) {
        best_priority = p->priority;
        best = p;
        found = 1;
      }
    }
  }
  
  if(found) {
    last_selected_idx = best - proc;  // 保存本次选择的索引
  }
  
  return best;
}

// Aging机制：防止进程饥饿
void
aging_update(void)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    if(p->state == RUNNABLE) {
      p->wait_time++;
      
      // 如果等待时间超过阈值，提升优先级
      if(p->wait_time >= AGING_THRESHOLD) {
        if(p->priority < MAX_PRIORITY) {
          p->priority += AGING_BOOST;
          printf("[AGING] Process %d (%s): priority boosted to %d\n", 
                 p->pid, p->name, p->priority);
        }
        p->wait_time = 0;  // 重置等待时间
      }
    }
  }
}

// 进程主动让出CPU
void
yield(void)
{
  struct proc *p = myproc();
  
  if(p) {
    p->state = RUNNABLE;
    p->wait_time = 0;  // 重置等待时间
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

// 调度器 - 优先级调度算法（带Aging）
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  
  printf("调度器启动 - 优先级调度算法 (带Aging机制)\n");
  
  int idle_count = 0;
  int aging_counter = 0;
  
  for(;;){
    // 开启中断，允许设备中断
    intr_on();
    
    // 周期性执行aging更新
    aging_counter++;
    if(aging_counter >= 10) {  // 每10次调度循环执行一次aging
      aging_update();
      aging_counter = 0;
    }
    
    // 选择优先级最高的可运行进程
    p = select_highest_priority();
    
    // 如果找到可运行的进程，切换过去
    if(p) {
      idle_count = 0;  // 重置空闲计数
      
      p->state = RUNNING;
      p->wait_time = 0;  // 重置等待时间
      c->proc = p;
      
      // 切换到进程
      swtch(&c->context, &p->context);
      
      // 进程切换回来后
      c->proc = 0;
      
      // 更新进程统计信息
      if(p->state == RUNNABLE || p->state == RUNNING) {
        p->ticks++;  // 增加CPU使用时间
      }
      
    } else {
      // 没有可运行的进程
      idle_count++;
      if(idle_count % 100000000 == 0) {
        printf("[SCHEDULER] No runnable processes (idle count: %d)\n", idle_count);
      }
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
  p->wait_time = 0;  // 睡眠时重置等待时间

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
      p->wait_time = 0;  // 唤醒时重置等待时间
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

  // 关闭所有打开的文件
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      fileclose(p->ofile[fd]);
      p->ofile[fd] = 0;
    }
  }

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
    havekids = 0;
    for(pp = proc; pp < &proc[NPROC]; pp++){
      if(pp->parent == p){
        havekids = 1;
        if(pp->state == ZOMBIE){
          pid = pp->pid;
          if(status != 0)
            *status = pp->xstate;
          freeproc(pp);
          return pid;
        }
      }
    }

    if(!havekids){
      return -1;
    }

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
        p->state = RUNNABLE;
      }
      return 0;
    }
  }
  return -1;
}

// 进程状态调试
void 
debug_proc_table(void) {
  static char *states[] = {
      [UNUSED]   "UNUSED",
      [USED]     "USED",
      [SLEEPING] "SLEEPING",
      [RUNNABLE] "RUNNABLE",
      [RUNNING]  "RUNNING",
      [ZOMBIE]   "ZOMBIE"
  };
  
  printf("\n=== Process Table ===\n");
  printf("PID\tPriority\tTicks\tWait\tState\t\tName\n");
  printf("------------------------------------------------------------------\n");

  for (int i = 0; i < NPROC; i++) {
      struct proc *p = &proc[i];
      if (p->state != UNUSED) {
          char *state_str = "UNKNOWN";
          if(p->state >= UNUSED && p->state <= ZOMBIE) {
              state_str = states[p->state];
          }

          printf("%d\t%d\t\t%d\t%d\t%s\t\t%s\n",
                 p->pid, p->priority, p->ticks, p->wait_time, 
                 state_str, p->name);
      }
  }
  printf("==================================================================\n\n");
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