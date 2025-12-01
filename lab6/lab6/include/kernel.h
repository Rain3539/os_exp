/**
 * Unified Kernel Header File
 * Contains all necessary definitions for the kernel
 */

#ifndef KERNEL_H
#define KERNEL_H

#include "types.h"
#include "param.h"
#include "defs.h"      // 包含 defs.h 来获取函数声明
#include "proc.h"      // 包含 proc.h 来获取结构体定义
#include "spinlock.h"  // 包含 spinlock 定义
#include "interrupt.h" // 包含中断定义

// 全局变量声明
extern struct cpu cpus[NCPU];
extern struct devsw devsw[NDEV];

#endif /* KERNEL_H */