// 系统调用号定义
#ifndef SYSCALL_H
#define SYSCALL_H

// 进程管理相关系统调用
#define SYS_EXIT        1
#define SYS_GETPID      2
#define SYS_FORK        3
#define SYS_WAIT        4
#define SYS_SBRK        10
#define SYS_KILL        11

// 文件操作相关系统调用
#define SYS_READ        5
#define SYS_WRITE       6
#define SYS_OPEN        7
#define SYS_CLOSE       8
#define SYS_UNLINK      12
#define SYS_MKDIR       13

// 优先级调度相关系统调用
#define SYS_SETPRIORITY 14
#define SYS_GETPRIORITY 15

// 其他系统调用
#define SYS_EXEC        9

#endif // SYSCALL_H