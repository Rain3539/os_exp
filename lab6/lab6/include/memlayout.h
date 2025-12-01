/**
 * 内存布局头文件
 * 定义RISC-V系统的内存地址和布局
 */

#ifndef MEMLAYOUT_H
#define MEMLAYOUT_H

// QEMU虚拟机器内存布局
#define KERNBASE 0x80000000L                    // 内核基地址
#define PHYSTOP (KERNBASE + 128*1024*1024)     // 物理内存顶部

// 内存映射I/O地址
#define UART0 0x10000000L        // UART0地址
#define VIRTIO0 0x10008000L      // VIRTIO设备地址
#define PLIC 0x0c000000L         // 中断控制器地址
#define KERNEL_HEAP_SIZE (128 * 1024 * 1024)  // 内核堆大小

// 页大小
#define PGSIZE 4096  // 页面大小4KB

// 蹦床页面地址 (用于用户-内核转换)
#define TRAMPOLINE 0x3ffffff000UL

// 创建页表条目
#define MAKE_SATP(pagetable) ((((uint64)(pagetable)) >> 12) | (8L << 60))

#endif /* MEMLAYOUT_H */
