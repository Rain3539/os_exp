// missing_defs.h - 提供缺失的函数定义和宏
#ifndef MISSING_DEFS_H
#define MISSING_DEFS_H

#include "types.h"
#include "proc.h"

#define UART0_IRQ 10         // UART中断号
#define VIRTIO0_IRQ 1        // VIRTIO中断号
#define MAKE_SATP(pagetable) (((uint64)(pagetable) >> 12) | (8L << 60))

// 缺失的函数声明
void kexit(int status);
void setkilled(struct proc *p);
void intr_on(void);
void intr_off(void);
void syscall(void);
int vmfault(pagetable_t pagetable, uint64 stval, int is_load);
void prepare_return(void);
int intr_get(void);
void clockintr(void);
int cpuid(void);
int plic_claim(void);
void uartintr(void);
void virtio_disk_intr(void);
void plic_complete(int irq);
uint64 r_satp(void);
uint64 r_tp(void);

#endif /* MISSING_DEFS_H */