#ifndef FILE_H
#define FILE_H

#include "../type.h"
#include "fs.h"

// 内存中打开的文件
struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // 引用计数
  char readable;
  char writable;
  struct inode *ip;  // FD_INODE 和 FD_DEVICE
  uint off;           // FD_INODE
  short major;        // FD_DEVICE
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define mkdev(m,n)  ((uint)((m)<<16| (n)))

// 内存中的inode表（在fs.c中定义）
struct itable_struct {
  struct inode inode[50];  // NINODE = 50
};
extern struct itable_struct itable;

// 打开文件表（在file.c中定义）
struct ftable_struct {
  struct file file[100];  // NFILE = 100
};
extern struct ftable_struct ftable;

// -----------------------------------------------------------------
// *** 新增：fileinit 声明 ***
// -----------------------------------------------------------------
void fileinit(void);
// -----------------------------------------------------------------


// 文件系统调用使用的标志
#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200
#define O_TRUNC   0x400

#endif