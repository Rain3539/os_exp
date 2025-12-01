/**
 * Inode头文件
 * 包含inode结构体定义
 */

#ifndef INODE_H
#define INODE_H

#include "types.h"
#include "fs.h"
#include "sleeplock.h"

// 内存中的inode副本
struct inode {
    uint dev;                   // 设备号
    uint inum;                  // inode编号
    int ref;                    // 引用计数
    struct sleeplock lock;      // 保护以下字段的睡眠锁
    int valid;                  // inode是否已从磁盘读取

    short type;                 // 磁盘inode类型副本
    short major;                // 主设备号
    short minor;                // 次设备号
    short nlink;                // 链接数
    uint size;                  // 文件大小
    uint addrs[NDIRECT+1];      // 数据块地址数组
};

#endif /* INODE_H */