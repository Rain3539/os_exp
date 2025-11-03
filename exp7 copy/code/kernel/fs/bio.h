#ifndef BIO_H
#define BIO_H

#include "../type.h"

// 块缓存
struct buf {
  int valid;   // 缓冲区是否已从磁盘读取？
  int disk;    // 缓冲区是否需要写回磁盘？
  uint dev;
  uint blockno;
  uint refcnt;
  struct buf *prev; // LRU缓存列表
  struct buf *next;
  uchar data[1024];  // BSIZE = 1024
};

// bio.c 函数声明
void            binit(void);
struct buf*     bread(uint dev, uint blockno);
void            bwrite(struct buf *b);
void            brelse(struct buf *b);
void            bpin(struct buf *b);
void            bunpin(struct buf *b);

#endif