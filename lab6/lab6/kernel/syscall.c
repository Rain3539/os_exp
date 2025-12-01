// syscall.c - 系统调用实现框架

#include "kernel.h"
#include "syscall.h"
#include "proc.h"
#include "defs.h"
#include "printf.h"
#include "vm.h"
#include "file.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"

// 前向声明系统调用实现
extern int sys_fork(void);
extern int sys_exit(void);
extern int sys_wait(void);
extern int sys_pipe(void);
extern int sys_read(void);
extern int sys_kill(void);
extern int sys_exec(void);
extern int sys_fstat(void);
extern int sys_chdir(void);
extern int sys_dup(void);
extern int sys_getpid(void);
extern int sys_sbrk(void);
extern int sys_sleep(void);
extern int sys_uptime(void);
extern int sys_open(void);
extern int sys_write(void);
extern int sys_mknod(void);
extern int sys_unlink(void);
extern int sys_link(void);
extern int sys_mkdir(void);
extern int sys_close(void);

// 系统调用函数指针数组
static int (*syscalls[])(void) = {
    [SYS_fork]    sys_fork,
    [SYS_exit]    sys_exit,
    [SYS_wait]    sys_wait,
    [SYS_pipe]    sys_pipe,
    [SYS_read]    sys_read,
    [SYS_kill]    sys_kill,
    [SYS_exec]    sys_exec,
    [SYS_fstat]   sys_fstat,
    [SYS_chdir]   sys_chdir,
    [SYS_dup]     sys_dup,
    [SYS_getpid]  sys_getpid,
    [SYS_sbrk]    sys_sbrk,
    [SYS_sleep]   sys_sleep,
    [SYS_uptime]  sys_uptime,
    [SYS_open]    sys_open,
    [SYS_write]   sys_write,
    [SYS_mknod]   sys_mknod,
    [SYS_unlink]  sys_unlink,
    [SYS_link]    sys_link,
    [SYS_mkdir]   sys_mkdir,
    [SYS_close]   sys_close,
};

// 调试标志（可通过外部修改）
static int debug_syscalls = 1;  // 默认启用调试输出

// 系统调用名称（用于调试）
static const char *syscall_names[] = {
    [SYS_fork]    "fork",
    [SYS_exit]    "exit",
    [SYS_wait]    "wait",
    [SYS_pipe]    "pipe",
    [SYS_read]    "read",
    [SYS_kill]    "kill",
    [SYS_exec]    "exec",
    [SYS_fstat]   "fstat",
    [SYS_chdir]   "chdir",
    [SYS_dup]     "dup",
    [SYS_getpid]  "getpid",
    [SYS_sbrk]    "sbrk",
    [SYS_sleep]   "sleep",
    [SYS_uptime]  "uptime",
    [SYS_open]    "open",
    [SYS_write]   "write",
    [SYS_mknod]   "mknod",
    [SYS_unlink]  "unlink",
    [SYS_link]    "link",
    [SYS_mkdir]   "mkdir",
    [SYS_close]   "close",
};

// 获取当前进程的系统调用参数
// 参数n表示第n个参数（从0开始）
// a0-a5寄存器存储前6个参数
static struct trapframe*
get_trapframe(void)
{
    struct proc *p = myproc();
    if (!p || !p->trapframe) {
        return 0;
    }
    return p->trapframe;
}

// 提取整数参数（第n个参数，存储在a0-a5中）
int argint(int n, int *ip)
{
    struct trapframe *tf = get_trapframe();
    if (!tf) {
        if (debug_syscalls) {
            printf("[argint] 错误: 无trapframe\n");
        }
        return -1;
    }

    // a0-a5对应参数0-5
    uint64 *args = &tf->a0;
    if (n < 0 || n >= 6) {
        if (debug_syscalls) {
            printf("[argint] 错误: 参数索引 %d 超出范围 [0-5]\n", n);
        }
        return -1;
    }
    *ip = (int)args[n];
    if (debug_syscalls) {
        printf("[argint] 参数[%d] = %d (0x%lx)\n", n, *ip, args[n]);
    }
    return 0;
}

// 提取地址参数（第n个参数）
int argaddr(int n, uint64 *ip)
{
    struct trapframe *tf = get_trapframe();
    if (!tf) {
        if (debug_syscalls) {
            printf("[argaddr] 错误: 无trapframe\n");
        }
        return -1;
    }

    uint64 *args = &tf->a0;
    if (n < 0 || n >= 6) {
        if (debug_syscalls) {
            printf("[argaddr] 错误: 参数索引 %d 超出范围 [0-5]\n", n);
        }
        return -1;
    }
    *ip = args[n];
    if (debug_syscalls) {
        printf("[argaddr] 参数[%d] = 0x%lx\n", n, *ip);
    }
    return 0;
}

// 提取字符串参数（从用户地址空间）
int argstr(int n, char *buf, int max)
{
    uint64 addr;
    if (argaddr(n, &addr) < 0) {
        if (debug_syscalls) {
            printf("[argstr] 错误: 无法获取地址参数[%d]\n", n);
        }
        return -1;
    }
    int ret = fetchstr(addr, buf, max);
    if (debug_syscalls) {
        if (ret >= 0) {
            printf("[argstr] 参数[%d] = \"%s\" (addr=0x%lx, len=%d)\n", n, buf, addr, ret);
        } else {
            printf("[argstr] 错误: 无法从地址 0x%lx 获取字符串\n", addr);
        }
    }
    return ret;
}

// 从用户地址空间获取一个uint64值
int fetchaddr(uint64 addr, uint64 *ip)
{
    struct proc *p = myproc();
    if (!p) return -1;

    // 检查地址是否有效（在用户空间）
    // 简化实现：假设如果pagetable存在，则地址有效
    // 实际应该检查地址范围
    pagetable_t pagetable = p->pagetable;

    // 如果没有用户页表，可能是内核进程直接调用
    // 在这种情况下，直接访问地址（简化实现）
    if (!pagetable) {
        // 内核空间直接访问
        if (addr < KERNBASE || addr >= PHYSTOP) {
            return -1;
        }
        *ip = *(uint64*)addr;
        return 0;
    }

    // 用户空间：需要通过页表访问
    pte_t *pte = walk_lookup(pagetable, addr);
    if (!pte || !(*pte & PTE_V)) {
        return -1;  // 页面不存在
    }
    if (!(*pte & PTE_U)) {
        return -1;  // 不是用户页面
    }
    
    uint64 pa = PTE_PA(*pte) + (addr & (PGSIZE - 1));
    *ip = *(uint64*)pa;
    return 0;
}

// 从用户地址空间获取字符串（最多max字节）
int fetchstr(uint64 addr, char *buf, int max)
{
    struct proc *p = myproc();
    if (!p) return -1;

    if (max <= 0) return -1;
    
    pagetable_t pagetable = p->pagetable;
    
    // 简化实现：直接访问（如果在内核空间）或通过copyinstr
    if (!pagetable) {
        // 内核空间直接访问
        if (addr < KERNBASE || addr >= PHYSTOP) {
            return -1;
        }
        char *src = (char*)addr;
        int i = 0;
        for (i = 0; i < max - 1 && src[i] != 0; i++) {
            buf[i] = src[i];
        }
        buf[i] = 0;
        return i;
    }

    // 用户空间：使用copyinstr
    return copyinstr(pagetable, buf, addr, max);
}

// 辅助函数：通过页表获取物理地址（在使用前定义）
static uint64 walkaddr(pagetable_t pagetable, uint64 va)
{
    pte_t *pte = walk_lookup(pagetable, va);
    if (!pte || !(*pte & PTE_V)) {
        return 0;
    }
    if ((*pte & PTE_U) == 0) {
        return 0;  // 不是用户页面
    }
    return PTE_PA(*pte) + (va & (PGSIZE - 1));
}

// 内存复制函数（简化实现，在使用前定义）
static void memmove(void *dst, const void *src, uint64 n)
{
    const char *s = (const char *)src;
    char *d = (char *)dst;
    if (s < d && s + n > d) {
        // 重叠：从后往前复制
        s += n;
        d += n;
        while (n-- > 0)
            *--d = *--s;
    } else {
        // 不重叠：从前往后复制
        while (n-- > 0)
            *d++ = *s++;
    }
}

// 从用户空间复制数据到内核空间
int copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
    uint64 n, va0, pa0;
    
    while (len > 0) {
        va0 = PGROUNDDOWN(srcva);
        pa0 = walkaddr(pagetable, va0);
        if (pa0 == 0)
            return -1;
        n = PGSIZE - (srcva - va0);
        if (n > len)
            n = len;
        memmove(dst, (void *)(pa0 + (srcva - va0)), n);

        len -= n;
        dst += n;
        srcva = va0 + PGSIZE;
    }
    return 0;
}

// 从用户空间复制字符串到内核空间
int copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
    uint64 n, va0, pa0;
    int got_null = 0;

    while (got_null == 0 && max > 0) {
        va0 = PGROUNDDOWN(srcva);
        pa0 = walkaddr(pagetable, va0);
        if (pa0 == 0)
            return -1;
        n = PGSIZE - (srcva - va0);
        if (n > max)
            n = max;

        char *p = (char *)(pa0 + (srcva - va0));
        while (n > 0) {
            if (*p == '\0') {
                *dst = '\0';
                got_null = 1;
                break;
            } else {
                *dst = *p;
            }
            --n;
            --max;
            p++;
            dst++;
        }

        srcva = va0 + PGSIZE;
    }
    if (got_null) {
        return 0;
    } else {
        return -1;
    }
}

// 从内核空间复制数据到用户空间
int copyout(pagetable_t pagetable, uint64 dsta, char *src, uint64 len)
{
    uint64 n, va0, pa0;

    while (len > 0) {
        va0 = PGROUNDDOWN(dsta);
        pa0 = walkaddr(pagetable, va0);
        if (pa0 == 0)
            return -1;
        n = PGSIZE - (dsta - va0);
        if (n > len)
            n = len;
        memmove((void *)(pa0 + (dsta - va0)), src, n);

        len -= n;
        src += n;
        dsta = va0 + PGSIZE;
    }
    return 0;
}

// 系统调用分发器
void syscall(void)
{
    struct proc *p = myproc();
    if (!p || !p->trapframe) {
        printf("[syscall] 无当前进程或trapframe\n");
        return;
    }

    struct trapframe *tf = p->trapframe;
    int num = tf->a7;  // 系统调用号在a7寄存器

    // 调试输出
    if (debug_syscalls) {
        const char *name = (num > 0 && num < NSYSCALL) ? syscall_names[num] : "unknown";
        printf("PID %d: syscall %d (%s)\n", p->pid, num, name);
    }

    // 检查系统调用号是否有效
    if (num > 0 && num < NSYSCALL && syscalls[num]) {
        // 调用系统调用函数
        int ret = syscalls[num]();
        // 返回值存储在a0寄存器
        tf->a0 = ret;
        
        // 调试输出返回值
        if (debug_syscalls) {
            printf("  -> returned: %d\n", ret);
        }
    } else {
        // 无效的系统调用号
        printf("[syscall] PID %d: 无效的系统调用号 %d\n", p->pid, num);
        tf->a0 = -1;
    }
}

