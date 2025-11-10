// 系统调用分发器
// 负责根据系统调用号分发到具体的处理函数

#include "../def.h"
#include "syscall.h"

// 外部系统调用函数声明
extern uint64 sys_exit(void);
extern uint64 sys_getpid(void);
extern uint64 sys_fork(void);
extern uint64 sys_wait(void);
extern uint64 sys_read(void);
extern uint64 sys_write(void);
extern uint64 sys_open(void);
extern uint64 sys_close(void);
extern uint64 sys_exec(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_kill(void);
extern uint64 sys_unlink(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_setpriority(void);
extern uint64 sys_getpriority(void);

// 系统调用函数指针数组
static uint64 (*syscalls[])(void) = {
    [SYS_EXIT]        = sys_exit,
    [SYS_GETPID]      = sys_getpid,
    [SYS_FORK]        = sys_fork,
    [SYS_WAIT]        = sys_wait,
    [SYS_READ]        = sys_read,
    [SYS_WRITE]       = sys_write,
    [SYS_OPEN]        = sys_open,
    [SYS_CLOSE]       = sys_close,
    [SYS_EXEC]        = sys_exec,
    [SYS_SBRK]        = sys_sbrk,
    [SYS_KILL]        = sys_kill,
    [SYS_UNLINK]      = sys_unlink,
    [SYS_MKDIR]       = sys_mkdir,
    [SYS_SETPRIORITY] = sys_setpriority,
    [SYS_GETPRIORITY] = sys_getpriority,
};

// 系统调用名称（用于调试）
static char *syscall_names[] = {
    [SYS_EXIT]        "exit",
    [SYS_GETPID]      "getpid",
    [SYS_FORK]        "fork",
    [SYS_WAIT]        "wait",
    [SYS_READ]        "read",
    [SYS_WRITE]       "write",
    [SYS_OPEN]        "open",
    [SYS_CLOSE]       "close",
    [SYS_EXEC]        "exec",
    [SYS_SBRK]        "sbrk",
    [SYS_KILL]        "kill",
    [SYS_UNLINK]      "unlink",
    [SYS_MKDIR]       "mkdir",
    [SYS_SETPRIORITY] "setpriority",
    [SYS_GETPRIORITY] "getpriority",
};

// 系统调用处理函数
void handle_syscall(struct trapframe *tf) {
    uint64 syscall_num = tf->a7;
    
    // 检查系统调用号是否有效
    if(syscall_num > 0 && 
       syscall_num < sizeof(syscalls)/sizeof(syscalls[0]) && 
       syscalls[syscall_num]) {
        
        // 打印系统调用信息（调试用）
        if(syscall_num < sizeof(syscall_names)/sizeof(syscall_names[0]) && 
           syscall_names[syscall_num]) {
            printf("[SYSCALL] %s (num=%d)\n", syscall_names[syscall_num], syscall_num);
        }
        
        // 调用对应的系统调用函数
        tf->a0 = syscalls[syscall_num]();
        
    } else {
        printf("[SYSCALL] Unknown system call: %d\n", syscall_num);
        tf->a0 = -1;
    }
    
    // 系统调用完成后，EPC需要指向下一条指令
    tf->epc += 4;
}