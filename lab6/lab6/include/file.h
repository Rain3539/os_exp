/**
 * 文件头文件
 * 包含文件系统文件定义
 */

#ifndef FILE_H
#define FILE_H

#include "types.h"
#include "pipe.h"
#include "inode.h"

// 文件结构体
struct file {
    enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;  // 文件类型
    int ref;              // 引用计数
    char readable;        // 可读标志
    char writable;        // 可写标志
    struct pipe *pipe;    // 管道指针 (FD_PIPE类型)
    struct inode *ip;     // inode指针 (FD_INODE和FD_DEVICE类型)
    uint off;             // 文件偏移量 (FD_INODE类型)
    short major;          // 主设备号 (FD_DEVICE类型)
};

// 设备号操作宏
#define major(dev)  ((dev) >> 16 & 0xFFFF)        // 提取主设备号
#define minor(dev)  ((dev) & 0xFFFF)              // 提取次设备号
#define mkdev(m,n)  ((uint)((m) << 16 | (n)))     // 组合设备号

#endif /* FILE_H */