#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

// 系统调用号定义
#define SYS_fork    1
#define SYS_exit    2
#define SYS_wait    3
#define SYS_pipe    4
#define SYS_read    5
#define SYS_kill    6
#define SYS_exec    7
#define SYS_fstat   8
#define SYS_chdir   9
#define SYS_dup     10
#define SYS_getpid  11
#define SYS_sbrk    12
#define SYS_sleep   13
#define SYS_uptime  14
#define SYS_open    15
#define SYS_write   16
#define SYS_mknod   17
#define SYS_unlink  18
#define SYS_link    19
#define SYS_mkdir   20
#define SYS_close   21

// 系统调用最大数量
#define NSYSCALL 22

// 系统调用函数声明
int sys_fork(void);
int sys_exit(void);
int sys_wait(void);
int sys_pipe(void);
int sys_read(void);
int sys_kill(void);
int sys_exec(void);
int sys_fstat(void);
int sys_chdir(void);
int sys_dup(void);
int sys_getpid(void);
int sys_sbrk(void);
int sys_sleep(void);
int sys_uptime(void);
int sys_open(void);
int sys_write(void);
int sys_mknod(void);
int sys_unlink(void);
int sys_link(void);
int sys_mkdir(void);
int sys_close(void);

// 系统调用分发函数
void syscall(void);

// 参数提取函数
int argint(int n, int *ip);
int argaddr(int n, uint64 *ip);
int argstr(int n, char *buf, int max);

// 用户内存访问函数
int fetchaddr(uint64 addr, uint64 *ip);
int fetchstr(uint64 addr, char *buf, int max);
int copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len);
int copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max);
int copyout(pagetable_t pagetable, uint64 dsta, char *src, uint64 len);

#endif /* SYSCALL_H */

