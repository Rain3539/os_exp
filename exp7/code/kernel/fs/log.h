#ifndef LOG_H
#define LOG_H

#include "../type.h"

// 前置声明
struct buf;
struct superblock;

// 日志系统的简单实现
// 一个简单的日志文件系统，保证崩溃后的一致性

// 日志头部（写入日志区第一个块）
struct logheader {
  int n;              // 日志中的块数量
  int block[30];      // 日志中每个块在磁盘中的实际块号（LOGSIZE = 30）
};

// log.c 函数声明
void            initlog(int dev, struct superblock *sb);
void            begin_op(void);
void            end_op(void);
void            log_write(struct buf *b);

#endif