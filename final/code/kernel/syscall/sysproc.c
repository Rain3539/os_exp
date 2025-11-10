// 进程管理相关系统调用实现
// 包括：exit, getpid, fork, wait, setpriority, getpriority等

#include "../def.h"
#include "../proc/proc.h"
#include "syscall.h"

// 系统调用：进程退出
uint64 sys_exit(void) {
    struct proc *p = myproc();
    if(p) {
        printf("[SYS_EXIT] Process %d (%s) exiting\n", p->pid, p->name);
        exit(0);
    }
    return 0;
}

// 系统调用：获取当前进程ID
uint64 sys_getpid(void) {
    struct proc *p = myproc();
    if(p) {
        return p->pid;
    }
    return -1;
}

// 系统调用：创建一个新进程（克隆当前进程）
uint64 sys_fork(void) {
    printf("[SYS_FORK] Fork not fully implemented yet\n");
    // TODO: 实现进程创建逻辑，包括复制父进程的内存空间、文件描述符等
    return -1;
}

// 系统调用：等待子进程退出
uint64 sys_wait(void) {
    int status;
    return wait(&status);
}

// 系统调用：增加程序的堆内存空间
uint64 sys_sbrk(void) {
    printf("[SYS_SBRK] Sbrk not fully implemented yet\n");
    // TODO: 调整进程的数据段大小，用于动态内存分配
    return 0;
}

// 系统调用：杀死进程
uint64 sys_kill(void) {
    struct proc *p = myproc();
    if(!p) return -1;
    
    // 从trapframe获取参数：pid
    int pid = p->trapframe->a0;
    
    return kill(pid);
}

// 系统调用：设置进程优先级
// 参数：a0 = pid, a1 = priority
uint64 sys_setpriority(void) {
    struct proc *p = myproc();
    if(!p) return -1;
    
    // 从trapframe获取参数
    int pid = p->trapframe->a0;
    int priority = p->trapframe->a1;
    
    // 优先级范围检查
    if(priority < MIN_PRIORITY || priority > MAX_PRIORITY) {
        printf("[SYS_SETPRIORITY] Invalid priority %d (valid range: %d-%d)\n", 
               priority, MIN_PRIORITY, MAX_PRIORITY);
        return -1;
    }
    
    // 查找目标进程
    struct proc *target = 0;
    for(struct proc *pp = proc; pp < &proc[NPROC]; pp++) {
        if(pp->pid == pid && pp->state != UNUSED) {
            target = pp;
            break;
        }
    }
    
    if(!target) {
        printf("[SYS_SETPRIORITY] Process %d not found\n", pid);
        return -1;
    }
    
    // 设置优先级
    int old_priority = target->priority;
    target->priority = priority;
    
    printf("[SYS_SETPRIORITY] Process %d (%s): priority %d -> %d\n", 
           pid, target->name, old_priority, priority);
    
    return 0;
}

// 系统调用：获取进程优先级
// 参数：a0 = pid
uint64 sys_getpriority(void) {
    struct proc *p = myproc();
    if(!p) return -1;
    
    // 从trapframe获取参数
    int pid = p->trapframe->a0;
    
    // 查找目标进程
    for(struct proc *pp = proc; pp < &proc[NPROC]; pp++) {
        if(pp->pid == pid && pp->state != UNUSED) {
            return pp->priority;
        }
    }
    
    printf("[SYS_GETPRIORITY] Process %d not found\n", pid);
    return -1;
}

// 系统调用：执行一个新的程序
uint64 sys_exec(void) {
    printf("[SYS_EXEC] Exec not fully implemented yet\n");
    // TODO: 在当前进程的上下文中加载并执行一个新的程序
    return -1;
}