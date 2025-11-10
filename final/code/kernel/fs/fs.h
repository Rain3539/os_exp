#ifndef FS_H
#define FS_H

#include "../type.h"

// 前置声明
struct file;

// 文件系统参数
#define ROOTINO  1    // 根目录的inode号
#define BSIZE 1024    // 块大小（字节）

// 磁盘布局:
// [ boot block | super block | log | inode blocks | free bit map | data blocks ]

#define NDIRECT 12                   // 直接块数量
#define NINDIRECT (BSIZE / sizeof(uint))  // 间接块数量
#define MAXFILE (NDIRECT + NINDIRECT)   // 文件最大块数



// 磁盘上的inode结构
struct dinode {
  short type;           // 文件类型
  short major;          // 主设备号（T_DEVICE类型）
  short minor;          // 次设备号（T_DEVICE类型）
  short nlink;          // 指向此inode的目录项数量
  uint size;            // 文件大小（字节）
  uint addrs[NDIRECT+1];    // 数据块地址
};

// 每个块的inode数量
#define IPB             (BSIZE / sizeof(struct dinode))

// 包含inode i的块号
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// 每个块的位图位数
#define BPB             (BSIZE*8)

// 包含位b的位图块号
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

// 目录项是一个文件
#define DIRSIZ 14

// 目录项结构
struct dirent {
  ushort inum;          // inode号
  char name[DIRSIZ];    // 文件名
};

// 磁盘上的超级块结构
struct superblock {
  uint magic;       // 必须是FSMAGIC
  uint size;        // 文件系统大小（块）
  uint nblocks;     // 数据块数量
  uint ninodes;     // inode数量
  uint nlog;        // 日志块数量
  uint logstart;    // 第一个日志块
  uint inodestart;  // 第一个inode块
  uint bmapstart;   // 第一个空闲位图块
};

#define FSMAGIC 0x10203040

// 内存中的inode
struct inode {
  uint dev;           // 设备号
  uint inum;          // inode号
  int ref;            // 引用计数
  int valid;          // inode是否从磁盘读取

  short type;         // 从磁盘复制
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+1];
};

// inode类型
#define T_DIR     1   // 目录
#define T_FILE    2   // 文件
#define T_DEVICE  3   // 设备

// 表示一个文件系统
struct filesys {
  struct superblock sb;
  uint dev;
};

// 函数声明

// fs.c
void          fsinit(int dev);
struct inode* ialloc(uint dev, short type);
struct inode* iget(uint dev, uint inum);
struct inode* idup(struct inode *ip);
void          ilock(struct inode *ip);
void          iunlock(struct inode *ip);
void          iput(struct inode *ip);
void          iunlockput(struct inode *ip);
void          iupdate(struct inode *ip);
int           readi(struct inode *ip, int user_dst, uint64 dst, uint off, uint n);
int           writei(struct inode *ip, int user_src, uint64 src, uint off, uint n);
void          itrunc(struct inode *ip);
struct inode* namei(char *path);
struct inode* nameiparent(char *path, char *name);
int           dirlink(struct inode *dp, char *name, uint inum);
struct inode* dirlookup(struct inode *dp, char *name, uint *poff);
int           unlink(const char *path); 
// file.c
struct file* filealloc(void);
void          fileclose(struct file *f);
struct file* filedup(struct file *f);
int           fileread(struct file *f, uint64 addr, int n);
int           filestat(struct file *f, uint64 addr);
int           filewrite(struct file *f, uint64 addr, int n);
void          fileinit(void); 

// 获取超级块
void          readsb(int dev, struct superblock *sb);

#endif