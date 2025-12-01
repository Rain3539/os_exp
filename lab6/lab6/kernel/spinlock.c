/**
 * 自旋锁实现
 * 本实验的简化实现
 */

#include "kernel.h"

// 初始化自旋锁
void initlock(struct spinlock* lk, char* name) {
    lk->locked = 0;    // 初始状态未锁定
    lk->name = name;   // 锁名称
    lk->cpu = -1;      // 初始无CPU持有
}

// 获取自旋锁
void acquire(struct spinlock* lk) {
    // 简化实现：直接设置锁定标志
    // 在实际实现中，这将使用原子操作
    lk->locked = 1;
}

// 释放自旋锁
void release(struct spinlock* lk) {
    // 简化实现：直接清除锁定标志
    lk->locked = 0;
}

// 检查自旋锁是否被持有
int holding(struct spinlock* lk) {
    return lk->locked;
}