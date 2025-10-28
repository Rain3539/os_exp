// 块缓存实现
// 
// 块缓存是磁盘块的缓冲区副本，存储在内存中。
// 缓存提供了两个主要功能：
// 1) 同步访问磁盘块，确保每个块只有一个内核副本
//    且一次只有一个内核线程使用该副本。
// 2) 缓存常用块，这样它们不需要从慢速磁盘重新读取。

#include "../def.h"
#include "bio.h"
#include "fs.h"

#define NBUF 30  // 缓冲区数量

struct {
  struct buf buf[NBUF];
  
  // 双向循环链表，按最近使用排序
  // head.next 是最近使用的
  // head.prev 是最少使用的
  struct buf head;
} bcache;

// 初始化块缓存
void binit(void) {
  struct buf *b;

  // 创建链表
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  printf("块缓存初始化完成，缓冲区数量: %d\n", NBUF);
}

// 查找设备dev上的块blockno
// 如果未缓存，分配一个缓冲区
// 在任何情况下，返回锁定的缓冲区
static struct buf* bget(uint dev, uint blockno) {
  struct buf *b;

  // 块是否已缓存？
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      return b;
    }
  }

  // 未缓存
  // 回收最近最少使用(LRU)的未使用缓冲区
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      return b;
    }
  }
  
  panic("bget: 没有可用缓冲区");
  return 0;  // 不会执行到这里
}

// 返回设备dev上块blockno内容的锁定缓冲区
struct buf* bread(uint dev, uint blockno) {
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    // 这里应该从磁盘读取
    // 由于我们还没有实现磁盘驱动，暂时跳过
    // virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// 将b的内容写入磁盘。必须持有锁
void bwrite(struct buf *b) {
  // 这里应该写入磁盘
  // 由于我们还没有实现磁盘驱动，暂时跳过
  // virtio_disk_rw(b, 1);
  
  // 简单实现：标记为已写入
  b->disk = 0;
}

// 释放锁定的缓冲区
// 将缓冲区移到MRU位置
void brelse(struct buf *b) {
  b->refcnt--;
  if (b->refcnt == 0) {
    // 没有人在缓存这个块
    // 将其移到链表头部
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

// 增加缓冲区的引用计数
void bpin(struct buf *b) {
  b->refcnt++;
}

// 减少缓冲区的引用计数
void bunpin(struct buf *b) {
  b->refcnt--;
}