// sysproc.c - 进程相关系统调用实现

#include "kernel.h"
#include "syscall.h"
#include "proc.h"
#include "defs.h"
#include "printf.h"
#include "vm.h"
#include "param.h"

extern struct proc proc[NPROC];
extern struct proc* myproc(void);
extern int create_process(void (*entry)(void));
extern void exit_process(int status);
extern int wait_process(int *status);
extern void setkilled(struct proc* p);
extern uint64 get_time(void);

// sys_fork - 创建子进程（简化实现）
int sys_fork(void)
{
    struct proc *p = myproc();
    if (!p) {
        return -1;
    }
    
    // 简化实现：创建一个新进程，复制父进程的状态
    // 注意：这是一个简化实现，不能完全复制执行上下文
    // 真正的fork需要：
    //   1. 复制页表和内存空间
    //   2. 复制文件描述符表（增加引用计数）
    //   3. 复制进程上下文（寄存器状态）
    //   4. 让子进程从fork调用点继续执行（返回0）
    // 在我们的简化模型中，由于所有进程都需要入口函数，
    // 子进程无法真正从fork调用点继续执行
    
    // 分配新进程
    extern struct proc* alloc_process(void);
    struct proc *np = alloc_process();
    if (!np) {
        return -1;  // 无法分配进程
    }
    
    // 复制父进程的基本状态
    np->sz = p->sz;
    np->pagetable = p->pagetable;  // 简化：共享页表（实际应该复制）
    np->cwd = p->cwd;  // 简化：共享当前目录
    
    // 复制文件描述符表（增加引用计数）
    for (int i = 0; i < NOFILE; i++) {
        if (p->ofile[i]) {
            np->ofile[i] = p->ofile[i];
            // 简化：不处理引用计数（实际应该增加f->ref）
        }
    }
    
    // 设置父子关系
    np->parent = p;
    
    // 复制进程名
    for (int i = 0; i < 16; i++) {
        np->name[i] = p->name[i];
    }
    
    // 复制trapframe（如果存在）
    if (p->trapframe) {
        // 分配新的trapframe
        extern void* alloc_page(void);
        void* tf_page = alloc_page();
        if (tf_page) {
            np->trapframe = (struct trapframe*)tf_page;
            // 复制trapframe内容
            for (int i = 0; i < sizeof(struct trapframe)/sizeof(uint64); i++) {
                ((uint64*)np->trapframe)[i] = ((uint64*)p->trapframe)[i];
            }
            // 子进程的返回值应该是0
            np->trapframe->a0 = 0;
        } else {
            // 如果分配失败，使用静态fallback（仅用于测试）
            static struct trapframe static_tf;
            np->trapframe = &static_tf;
            for (int i = 0; i < sizeof(struct trapframe)/sizeof(uint64); i++) {
                ((uint64*)np->trapframe)[i] = ((uint64*)p->trapframe)[i];
            }
            np->trapframe->a0 = 0;
        }
    } else {
        // 如果父进程没有trapframe，为子进程分配一个
        extern void* alloc_page(void);
        void* tf_page = alloc_page();
        if (tf_page) {
            np->trapframe = (struct trapframe*)tf_page;
            // 初始化trapframe
            for (int i = 0; i < sizeof(struct trapframe)/sizeof(uint64); i++) {
                ((uint64*)np->trapframe)[i] = 0;
            }
            // 子进程的返回值应该是0
            np->trapframe->a0 = 0;
        }
    }
    
    // 设置进程名
    extern int snprintf(char *buf, int size, const char *fmt, ...);
    snprintf(np->name, sizeof(np->name), "fork-%d", np->pid);
    
    // 注意：子进程无法真正从fork调用点继续执行，
    // 因为我们的模型要求所有进程都需要入口函数
    // 所以这里不设置entry，子进程不会自动运行
    // 这是一个简化实现的限制
    
    // 标记为可运行（虽然子进程没有entry，但状态设置为RUNNABLE）
    // 实际运行时，子进程需要手动设置entry或使用其他机制
    np->state = RUNNABLE;
    extern volatile int need_resched;
    need_resched = 1;
    
    // 父进程返回子进程PID
    // 注意：子进程无法真正从fork调用点继续执行，
    // 因为我们的模型要求所有进程都需要入口函数
    // 子进程的返回值（0）已经设置在trapframe->a0中
    return np->pid;
}

// sys_exit - 退出当前进程
int sys_exit(void)
{
    int n;
    if (argint(0, &n) < 0)
        return -1;
    exit_process(n);
    return 0;  // 不会到达这里
}

// sys_wait - 等待子进程退出
int sys_wait(void)
{
    uint64 p;
    if (argaddr(0, &p) < 0)
        return -1;
    
    int status_val = 0;
    int *status = (p != 0) ? &status_val : 0;
    
    int pid = wait_process(status);
    
    // 如果有status指针，将退出状态复制回用户空间
    if (p != 0 && status) {
        struct proc *curproc = myproc();
        if (curproc && curproc->pagetable) {
            if (copyout(curproc->pagetable, p, (char*)status, sizeof(int)) < 0)
                return -1;
        } else {
            // 内核空间直接写入
            *(int*)p = *status;
        }
    }
    
    return pid;
}

// sys_kill - 杀死指定进程
int sys_kill(void)
{
    int pid;
    if (argint(0, &pid) < 0)
        return -1;
    
    struct proc *p;
    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->pid == pid) {
            setkilled(p);
            if (p->state == SLEEPING) {
                // 唤醒进程以便它可以看到被杀死
                p->state = RUNNABLE;
            }
            release(&p->lock);
            return 0;
        }
        release(&p->lock);
    }
    return -1;
}

// sys_getpid - 获取当前进程ID
int sys_getpid(void)
{
    struct proc *p = myproc();
    if (!p) return -1;
    return p->pid;
}

// sys_sbrk - 调整进程内存大小（简化实现）
int sys_sbrk(void)
{
    int addr;
    int n;
    struct proc *p = myproc();

    if (argint(0, &n) < 0)
        return -1;
    
    addr = p->sz;
    if (n > 0) {
        // 增长堆（简化实现：仅更新sz）
        p->sz += n;
    } else if (n < 0) {
        // 缩小堆（简化实现：仅更新sz）
        if (p->sz + n < 0) {
            return -1;
        }
        p->sz += n;
    }
    
    return addr;
}

// sys_sleep - 睡眠指定时间（毫秒）
int sys_sleep(void)
{
    int n;
    uint64 ticks0;

    if (argint(0, &n) < 0)
        return -1;
    
    struct proc *p = myproc();
    ticks0 = get_time();
    
    while (get_time() - ticks0 < n) {
        if (killed(p)) {
            return -1;
        }
        // 简单延时：实际应该使用sleep
        for (volatile int i = 0; i < 1000; i++);
    }
    
    return 0;
}

// sys_uptime - 获取系统运行时间
int sys_uptime(void)
{
    extern volatile uint64 ticks;
    return (int)ticks;
}

