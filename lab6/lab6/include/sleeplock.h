/**
 * 睡眠锁头文件
 * 包含睡眠锁数据结构和函数声明
 */

#ifndef SLEEPLOCK_H
#define SLEEPLOCK_H

#include "types.h"
#include "spinlock.h"

// 睡眠锁结构体
struct sleeplock {
    int locked;             // 锁是否被持有
    struct spinlock lk;     // 保护此睡眠锁的自旋锁
    char* name;             // 锁名称(调试用)
};

void initsleeplock(struct sleeplock* lk, char* name);  // 初始化睡眠锁
void acquiresleep(struct sleeplock* lk);               // 获取睡眠锁
void releasesleep(struct sleeplock* lk);               // 释放睡眠锁
int holdingsleep(struct sleeplock* lk);                // 检查睡眠锁是否被持有

#endif /* SLEEPLOCK_H */