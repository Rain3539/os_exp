/**
 * 本实验的简化实现
 * 包含各种内核函数的桩实现
 */

#include "kernel.h"

/*
 * 简化进程管理实现
 * - 基于 proc.h 中定义的 struct proc
 * - 将内核任务注册到现有的轮转调度器 (sched.c)
 * - 提供 create_process/alloc_process/wait_process 等接口
 */

#include "defs.h"
#include "proc.h"
#include "printf.h"

extern volatile int need_resched;

// 进程表（导出为全局符号 `proc`，与 xv6 风格一致）
struct proc proc[NPROC];

// 每个进程的内核栈
#define KSTACKSIZE 4096
static char kstacks[NPROC][KSTACKSIZE];
static int nextpid = 1;

// 懒初始化标志
static int proc_inited = 0;

// 全局等待锁，用于 wait()/sleep() 的协调（类似 xv6 的 wait_lock）
static struct spinlock wait_lock;

static void
proc_table_init(void)
{
    if (proc_inited)
        return;
    for (int i = 0; i < NPROC; i++) {
        proc[i].state = UNUSED;
        proc[i].pid = 0;
        initlock(&proc[i].lock, "proc_lock");
    }
    initlock(&wait_lock, "wait_lock");
    proc_inited = 1;
}

// trampol 函数：所有注册到轮转调度器的进程都会通过此函数进入
void proc_task_trampoline(void)
{
    struct proc *p = myproc();
    if (!p) {
        printf("[proc] trampoline: 无当前进程\n");
        panic("trampoline no proc");
    }

    if (p->entry) {
        p->entry(); // 调用进程入口函数
    }

    // 入口返回则视为正常退出
    exit_process(0);
    panic("proc_task_trampoline: exit returned");
}

// 从进程表中分配一个进程结构
struct proc*
alloc_process(void)
{
    proc_table_init();
    for (int i = 0; i < NPROC; i++) {
        if (proc[i].state == UNUSED) {
            proc[i].state = USED;
            proc[i].pid = nextpid++;
            proc[i].killed = 0;
            proc[i].xstate = 0;
            proc[i].parent = 0;
            proc[i].chan = 0;
            proc[i].sz = 0;
            proc[i].pagetable = 0;
            proc[i].trapframe = 0;
            // 分配并设置内核栈（栈顶）
            proc[i].kstack = (uint64)&kstacks[i][0];
            proc[i].entry = 0;
            for (int j = 0; j < NOFILE; j++) proc[i].ofile[j] = 0;
            proc[i].cwd = 0;
            for (int k = 0; k < 16; k++) proc[i].name[k] = 0;

            // 初始化上下文：把返回地址设为 trampoline，栈指针置为栈顶
            for (int r = 0; r < sizeof(struct context)/sizeof(uint64); r++) {
                ((uint64*)&proc[i].context)[r] = 0;
            }
            proc[i].context.ra = (uint64)proc_task_trampoline;
            // sp 指向栈顶（高地址）
            proc[i].context.sp = proc[i].kstack + KSTACKSIZE;

            return &proc[i];
        }
    }
    return 0; // 无空闲槽
}

void
free_process(struct proc* p)
{
    if (!p) return;
    p->state = UNUSED;
    p->pid = 0;
    p->parent = 0;
    p->chan = 0;
    p->entry = 0;
}

// 创建一个新的内核任务（进程）并注册到调度器
int
create_process(void (*entry)(void))
{
    proc_table_init();
    struct proc *p = alloc_process();
    if (!p) return -1;
    p->entry = entry;
    // 填写名字方便调试
    snprintf(p->name, sizeof(p->name), "kproc-%d", p->pid);

    // 将父设置为当前进程（可能为NULL，表示内核/主线程）
    p->parent = myproc();

    // 标记为可运行
    p->state = RUNNABLE;
    need_resched = 1;
    return p->pid;
}

// 当前正在运行的进程（基于调度器索引）
struct proc*
myproc(void)
{
    struct cpu *c = mycpu();
    return c ? c->proc : 0;
}

int
killed(struct proc* p)
{
    if (!p) return 0;
    return p->killed;
}

void
setkilled(struct proc* p)
{
    if (p) p->killed = 1;
}

// 退出函数（对当前进程生效）
void
exit_process(int status)
{
    struct proc* p = myproc();
    if (!p) return; // 非进程上下文则忽略

    acquire(&p->lock);
    p->xstate = status;
    p->state = ZOMBIE;
    wakeup(p->parent);
    need_resched = 1;
    sched();
    panic("exit_process: sched returned");
}

// 兼容旧接口 kexit -> exit_process
void
kexit(int status)
{
    exit_process(status);
}

// 等待任意子进程结束
int
wait_process(int *status)
{
    struct proc *curproc = myproc();

    // two cases: caller is a process (can sleep on wait_lock), or caller is kernel main (curproc==NULL)
    if (curproc) {
        acquire(&wait_lock);
        for (;;) {
            int havekids = 0;
            for (int i = 0; i < NPROC; i++) {
                struct proc *p = &proc[i];
                acquire(&p->lock);
                if (p->parent != curproc) {
                    release(&p->lock);
                    continue;
                }
                havekids = 1;
                if (p->state == ZOMBIE) {
                    int pid = p->pid;
                    if (status) *status = p->xstate;
                    free_process(p);
                    release(&p->lock);
                    release(&wait_lock);
                    return pid;
                }
                release(&p->lock);
            }

            if (!havekids) {
                release(&wait_lock);
                return -1; // 没有子进程
            }

            // 等待子进程变为 ZOMBIE
            sleep(curproc, &wait_lock);
        }
    } else {
        // 非进程上下文（例如 start() 中），使用循环 + scheduler_step 来等待
        for (;;) {
            int havekids = 0;
            for (int i = 0; i < NPROC; i++) {
                struct proc *p = &proc[i];
                // 不需要获取 p->lock，因为我们在单线程启动上下文，且 lock 不是并发的
                if (p->parent != 0) continue; // parent==NULL indicates main-thread parent
                havekids = 1;
                if (p->state == ZOMBIE) {
                    int pid = p->pid;
                    if (status) *status = p->xstate;
                    free_process(p);
                    return pid;
                }
            }

            if (!havekids) return -1;

            // 运行调度器一轮以让子进程执行
            need_resched = 1;
            scheduler_step();
        }
    }
}

// 睡眠/唤醒实现（简化）
void
sleep(void* chan, struct spinlock* lk)
{
    struct proc *p = myproc();
    if (!p)
        return;
    if (lk == 0)
        panic("sleep without lk");

    if (lk != &p->lock) {
        acquire(&p->lock);
        release(lk);
    }

    p->chan = chan;
    p->state = SLEEPING;
    need_resched = 1;
    sched();
    p->chan = 0;

    if (lk != &p->lock) {
        release(&p->lock);
        acquire(lk);
    }
}

void
wakeup(void* chan)
{
    struct proc *current = myproc();
    for (int i = 0; i < NPROC; i++) {
        struct proc *p = &proc[i];
        if (p == current)
            continue;
        acquire(&p->lock);
        if (p->state == SLEEPING && p->chan == chan) {
            p->chan = 0;
            p->state = RUNNABLE;
            need_resched = 1;
        }
        release(&p->lock);
    }
}

// 简单的内存复制函数（用于内核空间复制）
static void simple_memcpy(void *dst, const void *src, uint64 n)
{
    const char *s = (const char *)src;
    char *d = (char *)dst;
    while (n-- > 0) {
        *d++ = *s++;
    }
}

// 简化拷贝函数（使用copyin/copyout实现）
int
either_copyin(void* dst, int user_src, uint64 src, uint64 len)
{
    if (user_src) {
        // 从用户空间复制
        struct proc *p = myproc();
        if (!p || !p->pagetable) {
            // 没有页表，可能是内核空间，直接复制
            simple_memcpy(dst, (void*)src, len);
            return 0;
        }
        // 使用copyin从用户空间复制
        extern int copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len);
        if (copyin(p->pagetable, (char*)dst, src, len) < 0)
            return -1;
    } else {
        // 从内核空间复制，直接复制
        simple_memcpy(dst, (void*)src, len);
    }
    return 0;
}

int
either_copyout(int user_dst, uint64 dst, void* src, uint64 len)
{
    if (user_dst) {
        // 复制到用户空间
        struct proc *p = myproc();
        if (!p || !p->pagetable) {
            // 没有页表，可能是内核空间，直接复制
            simple_memcpy((void*)dst, src, len);
            return 0;
        }
        // 使用copyout复制到用户空间
        extern int copyout(pagetable_t pagetable, uint64 dsta, char *src, uint64 len);
        if (copyout(p->pagetable, dst, (char*)src, len) < 0)
            return -1;
    } else {
        // 复制到内核空间，直接复制
        simple_memcpy((void*)dst, src, len);
    }
    return 0;
}

// 简化中断函数
static int intr_enabled = 0;
void push_off(void) { }
void pop_off(void) { }
void intr_on(void) { intr_enabled = 1; }
void intr_off(void) { intr_enabled = 0; }
int intr_get(void) { return intr_enabled; }

// 全局CPU数组(简化)
struct cpu cpus[NCPU];