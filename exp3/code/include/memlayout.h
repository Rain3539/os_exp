// kernel/memlayout.h

#ifndef __MEMLAYOUT_H__
#define __MEMLAYOUT_H__

// Base address where the kernel is loaded in physical memory
#define KERNBASE 0x80000000L

// Physical address of the UART device
#define UART0 0x10000000L

// End of physical memory (for a machine with 128MB RAM)
#define PHYSTOP (KERNBASE + 128 * 1024 * 1024)

#endif // __MEMLAYOUT_H__