// 文件系统核心实现
//
// 此文件包含创建、读取、写入和删除磁盘上文件的底层例程
// 以及列出目录和解析路径名的例程。

#include "../def.h"
#include "fs.h"
#include "bio.h"
#include "log.h"
#include "file.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

// 必须有一个超级块的副本
struct superblock sb;

// 内存中的inode表（结构体在file.h中定义）
struct itable_struct itable;

// 读取超级块
void readsb(int dev, struct superblock *sb) {
     struct buf *bp;

     bp = bread(dev, 1);
     for(int i = 0; i < sizeof(*sb); i++) {
          ((char*)sb)[i] = bp->data[i];
     }
     brelse(bp);
}

// 初始化文件系统
void fsinit(int dev) {
     readsb(dev, &sb);
        
     // 如果魔数不正确，说明没有文件系统镜像或是第一次使用
     // 在简化实现中，我们在内存中初始化一个超级块用于测试
     if(sb.magic != FSMAGIC) {
          printf("未检测到有效的文件系统镜像，使用默认配置初始化...\n");
             
          // 初始化超级块（仅在内存中，用于测试）
          sb.magic = FSMAGIC;
          sb.size = 1000;                             // 文件系统大小：1000块
          sb.nlog = 30;                                  // 日志块数
          sb.logstart = 2;                         // 日志起始块号
          sb.ninodes = 200;                        // inode数量
             
          // 计算各区域大小
          uint ninodeblocks = sb.ninodes / IPB + 1;
          uint nbitmap = sb.size / (BSIZE * 8) + 1;
          uint nmeta = 2 + sb.nlog + ninodeblocks + nbitmap;
             
          sb.inodestart = 2 + sb.nlog;
          sb.bmapstart = 2 + sb.nlog + ninodeblocks;
          sb.nblocks = sb.size - nmeta;
             
          printf("文件系统初始化（内存模拟）:\n");
     } else {
          printf("文件系统初始化（从镜像加载）:\n");
     }
        
     printf("     大小: %d 块\n", sb.size);
     printf("     数据块: %d\n", sb.nblocks);
     printf("     inode数: %d\n", sb.ninodes);
     printf("     日志块: %d\n", sb.nlog);
        
     // 先初始化日志系统
     initlog(dev, &sb);
        
     // 然后初始化根目录inode（如果还不存在）
     // 需要在一个事务中进行
     begin_op();
        
     struct buf *bp = bread(dev, IBLOCK(ROOTINO, sb));
     struct dinode *dip = (struct dinode*)bp->data + ROOTINO % IPB;
        
     if(dip->type == 0) {
          printf("初始化根目录inode...\n");
          memset(dip, 0, sizeof(*dip));
          dip->type = T_DIR;
          dip->nlink = 1;
          dip->size = 0;
          log_write(bp);
     }
     brelse(bp);
        
     end_op();
}

// -----------------------------------------------------------------
// *** balloc (来自之前的修复) ***
// -----------------------------------------------------------------
// 从空闲位图中分配一个零磁盘块
// 返回块号
static uint balloc(uint dev) {
     int b, bi, m;
     struct buf *bp;

  // 计算第一个数据块的起始位置
  uint ninodeblocks = sb.ninodes / IPB + 1;
  uint nbitmap = sb.size / (BSIZE * 8) + 1;
  uint data_start = 2 + sb.nlog + ninodeblocks + nbitmap;

     bp = 0;
     for(b = 0; b < sb.size; b += BPB){
          bp = bread(dev, BBLOCK(b, sb));
          for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
      uint bnum = b + bi; // 实际块号

      // 跳过元数据区
      if(bnum < data_start) {
        continue;
      }

               m = 1 << (bi % 8);
               if((bp->data[bi/8] & m) == 0){     // 是否空闲？
                    bp->data[bi/8] |= m;     // 标记为使用中
                    log_write(bp);
                    brelse(bp);
                       
                    // 清零块
                    struct buf *zbp = bread(dev, bnum);
                    memset(zbp->data, 0, BSIZE);
                    log_write(zbp);
                    brelse(zbp);
                       
        // (移除 balloc 内部的 printf 以减少日志混乱)
                    return bnum;
               }
          }
          brelse(bp);
     }
        
     printf("balloc: 磁盘空间不足\n");
     return 0;
}
// -----------------------------------------------------------------

// 释放磁盘块
static void bfree(int dev, uint b) {
     struct buf *bp;
     int bi, m;

  // 计算第一个数据块的起始位置
  uint ninodeblocks = sb.ninodes / IPB + 1;
  uint nbitmap = sb.size / (BSIZE * 8) + 1;
  uint data_start = 2 + sb.nlog + ninodeblocks + nbitmap;

     // 添加边界检查
     if(b < data_start) {
          panic("bfree: 尝试释放元数据块");
     }
        
     if(b >= sb.size) {
    panic("bfree: 块号超出范围");
     }

     bp = bread(dev, BBLOCK(b, sb));
     bi = b % BPB;
     m = 1 << (bi % 8);
        
     if((bp->data[bi/8] & m) == 0) {
    panic("bfree: 释放空闲块");
     }
        
     bp->data[bi/8] &= ~m;
     log_write(bp);
     brelse(bp);
}

// Inodes.

// 分配inode在设备dev上。
struct inode* ialloc(uint dev, short type) {
     int inum;
     struct buf *bp;
     struct dinode *dip;

     for(inum = 1; inum < sb.ninodes; inum++){
          bp = bread(dev, IBLOCK(inum, sb));
          dip = (struct dinode*)bp->data + inum%IPB;
          if(dip->type == 0){     // 空闲inode
               memset(dip, 0, sizeof(*dip));
               dip->type = type;
               log_write(bp);         // 标记它为已分配
               brelse(bp);
                  
               struct inode *ip = iget(dev, inum);
               ip->type = type;
               ip->nlink = 0;
               ip->size = 0;
                  
               return ip;
          }
          brelse(bp);
     }
        
     printf("ialloc: 没有可用inode\n");
     return 0;
}

// 复制修改后的内存中的inode到磁盘
void iupdate(struct inode *ip) {
     struct buf *bp;
     struct dinode *dip;

     bp = bread(ip->dev, IBLOCK(ip->inum, sb));
     dip = (struct dinode*)bp->data + ip->inum%IPB;
     dip->type = ip->type;
     dip->major = ip->major;
     dip->minor = ip->minor;
     dip->nlink = ip->nlink;
     dip->size = ip->size;
        
     for(int i = 0; i < NDIRECT+1; i++)
          dip->addrs[i] = ip->addrs[i];
             
     log_write(bp);
     brelse(bp);
}

// 在inode表中查找inode编号为inum的inode
struct inode* iget(uint dev, uint inum) {
     struct inode *ip, *empty;

     // 是否已经在表中缓存？
     empty = 0;
     for(ip = &itable.inode[0]; ip < &itable.inode[50]; ip++){
          if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
               ip->ref++;
               return ip;
          }
          if(empty == 0 && ip->ref == 0)          // 记住空槽位
               empty = ip;
     }

     // 回收空的inode表条目
     if(empty == 0)
          panic("iget: 没有可用inode");

     ip = empty;
     ip->dev = dev;
     ip->inum = inum;
     ip->ref = 1;
     ip->valid = 0;

     return ip;
}

// 增加对内存中inode的引用计数
struct inode* idup(struct inode *ip) {
     ip->ref++;
     return ip;
}

// 锁定给定的inode
void ilock(struct inode *ip) {
     struct buf *bp;
     struct dinode *dip;

     if(ip == 0 || ip->ref < 1)
          panic("ilock");

     if(ip->valid == 0){
          bp = bread(ip->dev, IBLOCK(ip->inum, sb));
          dip = (struct dinode*)bp->data + ip->inum%IPB;
          ip->type = dip->type;
          ip->major = dip->major;
          ip->minor = dip->minor;
          ip->nlink = dip->nlink;
          ip->size = dip->size;
             
          for(int i = 0; i < NDIRECT+1; i++)
               ip->addrs[i] = dip->addrs[i];
                  
          brelse(bp);
          ip->valid = 1;
          if(ip->type == 0)
               panic("ilock: 无类型");
     }
}

// 解锁给定的inode
void iunlock(struct inode *ip) {
     if(ip == 0 || ip->ref < 1)
          panic("iunlock");
}

// 删除对内存中inode的引用
void iput(struct inode *ip) {
     if(ip->ref == 1 && ip->valid && ip->nlink == 0){
          // inode没有链接，没有其他引用：截断并释放
          ilock(ip);
             
          itrunc(ip);
          ip->type = 0;
          iupdate(ip);
          ip->valid = 0;
             
          iunlock(ip);
     }

     ip->ref--;
}

// 解锁并释放
void iunlockput(struct inode *ip) {
     iunlock(ip);
     iput(ip);
}

// Inode内容

// 返回inode ip的第n个数据块的磁盘块地址
static uint bmap(struct inode *ip, uint bn) {
     uint addr, *a;
     struct buf *bp;

     if(bn < NDIRECT){
          if((addr = ip->addrs[bn]) == 0){
               addr = balloc(ip->dev);
               if(addr == 0)
                    return 0;
               ip->addrs[bn] = addr;
          }
          return addr;
     }
     bn -= NDIRECT;

     if(bn < NINDIRECT){
          // 加载间接块，必要时分配
          if((addr = ip->addrs[NDIRECT]) == 0){
               addr = balloc(ip->dev);
               if(addr == 0)
                    return 0;
               ip->addrs[NDIRECT] = addr;
          }
          bp = bread(ip->dev, addr);
          a = (uint*)bp->data;
          if((addr = a[bn]) == 0){
               addr = balloc(ip->dev);
               if(addr){
                    a[bn] = addr;
                    log_write(bp);
               }
          }
          brelse(bp);
          return addr;
     }

     panic("bmap: 超出范围");
     return 0;
}

// 截断inode（丢弃内容）
void itrunc(struct inode *ip) {
     int i, j;
     struct buf *bp;
     uint *a;

     for(i = 0; i < NDIRECT; i++){
          if(ip->addrs[i]){
               bfree(ip->dev, ip->addrs[i]);
               ip->addrs[i] = 0;
          }
     }

     if(ip->addrs[NDIRECT]){
          bp = bread(ip->dev, ip->addrs[NDIRECT]);
          a = (uint*)bp->data;
          for(j = 0; j < NINDIRECT; j++){
               if(a[j])
                    bfree(ip->dev, a[j]);
          }
          brelse(bp);
          bfree(ip->dev, ip->addrs[NDIRECT]);
          ip->addrs[NDIRECT] = 0;
     }

     ip->size = 0;
     iupdate(ip);
}

// 从inode复制数据
int readi(struct inode *ip, int user_dst, uint64 dst, uint off, uint n) {
     uint tot, m;
     struct buf *bp;

     if(off > ip->size || off + n < off)
          return 0;
     if(off + n > ip->size)
          n = ip->size - off;

     for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
          uint addr = bmap(ip, off/BSIZE);
          if(addr == 0)
               break;
          bp = bread(ip->dev, addr);
          m = min(n - tot, BSIZE - off%BSIZE);
             
          // 简化版本：假设user_dst==0，直接复制到内核地址
          for(uint i = 0; i < m; i++){
               ((char*)dst)[i] = bp->data[off%BSIZE + i];
          }
             
          brelse(bp);
     }
     return tot;
}

// 写入数据到inode
int writei(struct inode *ip, int user_src, uint64 src, uint off, uint n) {
     uint tot, m;
     struct buf *bp;

     if(off > ip->size || off + n < off)
          return -1;
     if(off + n > MAXFILE*BSIZE)
          return -1;

     for(tot=0; tot<n; tot+=m, off+=m, src+=m){
          uint addr = bmap(ip, off/BSIZE);
          if(addr == 0) {
               printf("writei: bmap返回0，bn=%d\r\n", off/BSIZE);
               break;
          }
          bp = bread(ip->dev, addr);
          m = min(n - tot, BSIZE - off%BSIZE);
             
          // 简化版本：假设user_src==0，直接从内核地址复制
          for(uint i = 0; i < m; i++){
               bp->data[off%BSIZE + i] = ((char*)src)[i];
          }
             
          log_write(bp);
          brelse(bp);
     }

     if(off > ip->size)
          ip->size = off;

     // 将更改写入磁盘
     iupdate(ip);

     return tot;
}

// 目录

int namecmp(const char *s, const char *t) {
     while(*s && *t && *s == *t){
          s++;
          t++;
     }
     return *s - *t;
}


// -----------------------------------------------------------------
// *** 优化的 dirlookup ***
// -----------------------------------------------------------------
// 在目录中查找name的inode
// 如果找到，设置*poff为目录中字节偏移量
// 返回解锁的inode，否则返回0
struct inode* dirlookup(struct inode *dp, char *name, uint *poff) {
     uint off, inum;
     struct buf *bp;

     if(dp->type != T_DIR)
          panic("dirlookup 不是目录");

  // 每次循环读取一个数据块, 而不是一个 dirent
     for(off = 0; off < dp->size; off += BSIZE){
    // 获取目录的数据块
          bp = bread(dp->dev, bmap(dp, off / BSIZE));
    if(bp == 0)
        panic("dirlookup: bmap/bread failed");

    // 在内存中扫描这个块
    struct dirent *de = (struct dirent*)bp->data;
    for(int i = 0; i < (BSIZE / sizeof(struct dirent)); i++, de++){
      if(de->inum == 0) // 空槽
        continue;
        
      if(namecmp(name, de->name) == 0){
        // 找到条目
        if(poff)
          *poff = off + i * sizeof(struct dirent);
        inum = de->inum;
        
        brelse(bp);
        return iget(dp->dev, inum);
      }
    }
    // 未在此块中找到, 释放块
    brelse(bp);
     }

     return 0; // 未找到
}
// -----------------------------------------------------------------


// -----------------------------------------------------------------
// *** 优化的 dirlink ***
// -----------------------------------------------------------------
// 将新条目(name, inum)写入目录dp
int dirlink(struct inode *dp, char *name, uint inum) {
     int off;
     struct dirent de;
     struct inode *ip;
  struct buf *bp;

     // 1. 检查name是否已存在 (现在使用优化的 dirlookup)
     if((ip = dirlookup(dp, name, 0)) != 0){
          iput(ip);
          return -1;
     }

  // 2. 查找空的dirent (使用块扫描优化)
  uint empty_off = 0;
     for(off = 0; off < dp->size; off += BSIZE){
          bp = bread(dp->dev, bmap(dp, off / BSIZE));
    if(bp == 0)
        panic("dirlink: bmap/bread failed");
    
    struct dirent *de_scan = (struct dirent*)bp->data;
    for(int i = 0; i < (BSIZE / sizeof(struct dirent)); i++){
        if(de_scan[i].inum == 0) {
            // 找到空槽
            empty_off = off + i * sizeof(struct dirent);
            brelse(bp);
            goto slot_found;
        }
    }
    brelse(bp);
     }
  // 未在现有块中找到空槽，追加到末尾
  empty_off = dp->size;

slot_found:
  // 3. 增加被链接inode的 nlink (来自之前的修复)
  ip = iget(dp->dev, inum);
  if(ip == 0)
    panic("dirlink: iget failed");
    
  ilock(ip);
  ip->nlink++;
  iupdate(ip);
  iunlockput(ip); // 释放从 iget() 获得的引用

     // 4. 填充并写入新的 dirent
     memset(&de, 0, sizeof(de));
     de.inum = inum;
     int i;
     for(i = 0; i < DIRSIZ && name[i]; i++)
          de.name[i] = name[i];
        
     // 使用 writei 在找到的偏移量处写入
     if(writei(dp, 0, (uint64)&de, empty_off, sizeof(de)) != sizeof(de))
          return -1;

     return 0;
}
// -----------------------------------------------------------------


// 路径

// 复制下一个路径元素从path到name
static char* skipelem(char *path, char *name) {
     char *s;
     int len;

     while(*path == '/')
          path++;
     if(*path == 0)
          return 0;
     s = path;
     while(*path != '/' && *path != 0)
          path++;
     len = path - s;
     if(len >= DIRSIZ)
          len = DIRSIZ - 1;
     for(int i = 0; i < len; i++)
          name[i] = s[i];
     name[len] = 0;
     while(*path == '/')
          path++;
     return path;
}

// 查找并返回路径名的inode
static struct inode* namex(char *path, int nameiparent, char *name) {
     struct inode *ip, *next;

     if(*path == '/')
          ip = iget(1, ROOTINO);     // dev = 1
     else
          // 简化实现：总是从根目录开始
          ip = iget(1, ROOTINO);

     while((path = skipelem(path, name)) != 0){
          ilock(ip);
          if(ip->type != T_DIR){
               iunlockput(ip);
               return 0;
          }
          if(nameiparent && *path == '\0'){
               // 停止一层提前
               iunlock(ip);
               return ip;
          }
    // (现在调用优化的 dirlookup)
          if((next = dirlookup(ip, name, 0)) == 0){
               iunlockput(ip);
               return 0;
          }
          iunlockput(ip);
          ip = next;
     }
     if(nameiparent){
          iput(ip);
          return 0;
     }
     return ip;
}

struct inode* namei(char *path) {
     char name[DIRSIZ];
     return namex(path, 0, name);
}

struct inode* nameiparent(char *path, char *name) {
     return namex(path, 1, name);
}