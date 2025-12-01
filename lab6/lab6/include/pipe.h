/**
 * 管道头文件
 * 包含管道结构体定义
 */

#ifndef PIPE_H
#define PIPE_H

#include "types.h"
#include "spinlock.h"

#define PIPESIZE 512  // 管道大小

// 管道结构体
struct pipe {
    struct spinlock lock;  // 保护管道的自旋锁
    char data[PIPESIZE];   // 管道数据缓冲区
    uint nread;            // 已读取字节数
    uint nwrite;           // 已写入字节数
    int readopen;          // 读端是否打开
    int writeopen;         // 写端是否打开
};

#endif /* PIPE_H */