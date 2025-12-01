/**
 * 自旋锁头文件
 * 包含自旋锁数据结构和函数声明
 */

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "types.h"

// 自旋锁结构体
struct spinlock {
    int locked;       // 锁是否被持有
    char* name;       // 锁名称(调试用)
    int cpu;          // 持有锁的CPU
};

void initlock(struct spinlock* lk, char* name);  // 初始化自旋锁
void acquire(struct spinlock* lk);               // 获取自旋锁
void release(struct spinlock* lk);               // 释放自旋锁
int holding(struct spinlock* lk);                // 检查自旋锁是否被持有

#endif /* SPINLOCK_H */