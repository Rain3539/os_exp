
// sched.c - 简化的内核调度器

// 替换为基于 proc 的简单调度器，使用 swtch 实现上下文切换

#include "types.h"
#include "defs.h"
#include "proc.h"
#include "printf.h"
#include "spinlock.h"

volatile int need_resched = 0; // 由中断或其他触发设置

// 单 CPU 简化实现
struct cpu *
mycpu(void)
{
    return &cpus[0];
}

int
get_current_task(void)
{
    // 兼容旧接口：返回 CPU 上的 proc 索引（-1 表示无）
    struct cpu *c = mycpu();
    if (!c || !c->proc) return -1;
    // 找到 proc 在 proc[] 中的索引
    for (int i = 0; i < NPROC; i++) {
        if (&proc[i] == c->proc) return i;
    }
    return -1;
}

// 供外部注册内核任务时使用的兼容接口：返回 proc 索引
int register_task(void (*fn)(void))
{
    // 为兼容旧接口：直接创建一个进程
    return create_process(fn);
}

void
sched(void)
{
    struct proc *p = myproc();
    if (p == 0)
        panic("sched: no proc");
    if (!holding(&p->lock))
        panic("sched: lock not held");

    struct cpu *c = mycpu();
    swtch(&p->context, &c->context);
}

// 调度器：扫描 proc[]，找到 RUNNABLE 进程并切换
void
scheduler_step(void)
{
    if (!need_resched)
        return;

    struct cpu *c = mycpu();
    need_resched = 0;

    for (struct proc *p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state != RUNNABLE) {
            release(&p->lock);
            continue;
        }

        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);
        c->proc = 0;
        release(&p->lock);
    }
}

// yield: 将当前进程标记为 RUNNABLE 并触发调度
void
yield(void)
{
    struct proc *p = myproc();
    if (!p) return;

    acquire(&p->lock);
    p->state = RUNNABLE;
    need_resched = 1;
    sched();
    release(&p->lock);
}
