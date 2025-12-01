# usys.S - 用户态系统调用接口
# 提供系统调用的汇编桩代码

# 宏定义：生成系统调用桩代码
# SYSCALL(name, num) 生成一个名为name的函数，使用系统调用号num
.macro SYSCALL name num
    .global \name
\name:
    li a7, \num
    ecall
    ret
.endm

# 宏定义：生成弱符号系统调用桩代码（用于与内核函数冲突的情况）
.macro SYSCALL_WEAK name num
    .weak \name
    .global \name
\name:
    li a7, \num
    ecall
    ret
.endm

# 生成所有系统调用的桩代码
SYSCALL fork, 1
SYSCALL exit, 2
SYSCALL wait, 3
SYSCALL pipe, 4
SYSCALL read, 5
SYSCALL kill, 6
SYSCALL exec, 7
SYSCALL fstat, 8
SYSCALL chdir, 9
SYSCALL dup, 10
SYSCALL getpid, 11
SYSCALL sbrk, 12
SYSCALL_WEAK sleep, 13  # 使用弱符号，避免与内核sleep函数冲突
SYSCALL uptime, 14
SYSCALL open, 15
SYSCALL write, 16
SYSCALL mknod, 17
SYSCALL unlink, 18
SYSCALL link, 19
SYSCALL mkdir, 20
SYSCALL close, 21

