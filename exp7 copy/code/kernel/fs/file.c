// 文件描述符管理
//
// 文件描述符层。
// 每个打开的文件由一个struct file表示，
// 它是包装inode或管道的包装器，加上I/O偏移量。
// 表ftable保存所有打开的文件。

#include "../def.h"
#include "file.h"
#include "fs.h"
#include "log.h"

// 全局打开文件表（结构体在file.h中定义）
struct ftable_struct ftable;

// 文件表初始化
void fileinit(void) {
  printf("文件表初始化完成\n");
}

// 为新打开的文件分配file结构
// 扫描文件表寻找未引用的文件(f->ref == 0)
struct file* filealloc(void) {
  struct file *f;

  for(f = ftable.file; f < ftable.file + 100; f++){
    if(f->ref == 0){
      f->ref = 1;
      return f;
    }
  }
  return 0;
}

// 增加文件的引用计数
struct file* filedup(struct file *f) {
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  return f;
}

// 关闭文件f（递减引用计数，在必要时关闭）
void fileclose(struct file *f) {
  struct file ff;

  if(f->ref < 1)
    panic("fileclose");
  
  f->ref--;
  if(f->ref > 0){
    return;
  }

  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;

  if(ff.type == FD_INODE || ff.type == FD_DEVICE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// 获取文件的元数据
int filestat(struct file *f, uint64 addr) {
  // 这里应该填充stat结构
  // 简化实现，暂不处理
  if(f->type == FD_INODE || f->type == FD_DEVICE){
    ilock(f->ip);
    // stati(f->ip, addr);
    iunlock(f->ip);
    return 0;
  }
  return -1;
}

// 从文件f读取
// addr是用户虚拟地址
int fileread(struct file *f, uint64 addr, int n) {
  int r = 0;

  if(f->readable == 0)
    return -1;

  if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, 1, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0)
      return -1;
    // 设备读取
    // r = devsw[f->major].read(1, addr, n);
  } else {
    panic("fileread");
  }

  return r;
}

// 写入文件f
// addr是用户虚拟地址
int filewrite(struct file *f, uint64 addr, int n) {
  int r, ret = 0;

  if(f->writable == 0)
    return -1;

  if(f->type == FD_INODE){
    // 一次写入几个块以避免超出
    // 单个日志事务的最大日志事务大小，
    // 包括inode、间接块、分配块和
    // 2个块的偏移
    int max = ((10 - 1 - 1 - 2) / 2) * BSIZE;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, 1, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();

      if(r != n1){
        // 错误从writei
        break;
      }
      i += r;
    }
    ret = (i == n ? n : -1);
  } else if(f->type == FD_DEVICE){
    if(f->major < 0)
      return -1;
    // 设备写入
    // ret = devsw[f->major].write(1, addr, n);
  } else {
    panic("filewrite");
  }

  return ret;
}