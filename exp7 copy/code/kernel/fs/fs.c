// =================================================================
// 文件系统核心实现 (fs.c)
//
// 此文件包含创建、读取、写入和删除磁盘上文件的底层例程
// 以及列出目录和解析路径名的例程。
//
// 这是一个分层的文件系统实现：
// (高层)  -> 路径名 (namei, nameiparent)
// (↓)     -> 目录 (dirlookup, dirlink)
// (↓)     -> 文件 I/O (readi, writei)
// (↓)     -> Inode 管理 (iget, iput, ialloc)
// (↓)     -> 块映射 (bmap, itrunc)
// (↓)     -> 块分配器 (balloc, bfree)
// (↓)     -> 日志 (log_write)
// (低层)  -> 块缓存 (bread, brelse)
// =================================================================

#include "../def.h"
#include "fs.h"
#include "bio.h"    // 块 I/O (缓冲区缓存)
#include "log.h"    // 日志
#include "file.h"   // 内存中的 inode 表

#define min(a, b) ((a) < (b) ? (a) : (b))

// 必须有一个超级块的副本
struct superblock sb;

// 内存中的inode表（结构体在file.h中定义）
struct itable_struct itable;

// =================================================================
// 第 1 层: 超级块与初始化
// =================================================================

/**
 * @brief 从磁盘读取超级块
 *
 * 超级块总是在磁盘的第 1 块。
 * @param dev 设备号
 * @param sb 指向内存中超级块结构的指针，用于填充数据
 */
void readsb(int dev, struct superblock *sb) {
     struct buf *bp;

     bp = bread(dev, 1); // 读取第 1 块
     for(int i = 0; i < sizeof(*sb); i++) {
          ((char*)sb)[i] = bp->data[i]; // 内存复制
     }
     brelse(bp); // 释放缓冲区
}

/**
 * @brief 初始化文件系统
 *
 * @param dev 设备号
 */
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
             
          // 计算各区域大小 (Boot=0, Super=1, Log=2..31)
          uint ninodeblocks = sb.ninodes / IPB + 1; // inode 块数
          uint nbitmap = sb.size / (BSIZE * 8) + 1;  // bitmap 块数
          uint nmeta = 2 + sb.nlog + ninodeblocks + nbitmap; // 元数据总块数
             
          sb.inodestart = 2 + sb.nlog; // inode 区起始
          sb.bmapstart = 2 + sb.nlog + ninodeblocks; // bitmap 区起始
          sb.nblocks = sb.size - nmeta; // 数据区大小
             
          printf("文件系统初始化（内存模拟）:\n");
     } else {
          printf("文件系统初始化（从镜像加载）:\n");
     }
       
     printf("     大小: %d 块\n", sb.size);
     printf("     数据块: %d\n", sb.nblocks);
     printf("     inode数: %d\n", sb.ninodes);
     printf("     日志块: %d\n", sb.nlog);
       
     // 先初始化日志系统（这会执行崩溃恢复）
     initlog(dev, &sb);
       
     // 然后初始化根目录inode（如果还不存在）
     // 需要在一个事务中进行，因为这会修改 inode 块
     begin_op();
       
     // 读取根 inode (ROOTINO 总是 1)
     struct buf *bp = bread(dev, IBLOCK(ROOTINO, sb));
     struct dinode *dip = (struct dinode*)bp->data + ROOTINO % IPB;
       
     if(dip->type == 0) { // 根目录还不存在
          printf("初始化根目录inode...\n");
          memset(dip, 0, sizeof(*dip));
          dip->type = T_DIR; // 类型为目录
          dip->nlink = 1;
          dip->size = 0;
          log_write(bp); // 将此更改写入日志
     }
     brelse(bp);
       
     end_op(); // 提交事务
}

// =================================================================
// 第 2 层: 块分配器 (Bitmap)
// =================================================================

/**
 * @brief 从空闲位图中分配一个零磁盘块
 * @return 成功则返回块号，失败则返回 0
 */
static uint balloc(uint dev) {
     int b, bi, m;
     struct buf *bp;

     // 计算第一个数据块的起始位置
     uint ninodeblocks = sb.ninodes / IPB + 1;
     uint nbitmap = sb.size / (BSIZE * 8) + 1;
     uint data_start = 2 + sb.nlog + ninodeblocks + nbitmap;

     bp = 0;
     // 遍历所有 bitmap 块
     for(b = 0; b < sb.size; b += BPB){
          bp = bread(dev, BBLOCK(b, sb));
          // 遍历块内的所有位
          for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
               uint bnum = b + bi; // 实际块号

               // 跳过元数据区 (引导块、超级块、日志、inode、bitmap)
               if(bnum < data_start) {
               continue;
               }

               m = 1 << (bi % 8); // 计算位掩码
               if((bp->data[bi/8] & m) == 0){     // 检查该位是否空闲 (== 0)
                    bp->data[bi/8] |= m;     // 标记为使用中
                    log_write(bp); // *日志操作 1*: 将 bitmap 块的更改写入日志
                    brelse(bp);
                   
                    // *日志操作 2*: 将新分配的块清零
                    struct buf *zbp = bread(dev, bnum);
                    memset(zbp->data, 0, BSIZE);
                    log_write(zbp); // 写入日志
                    brelse(zbp);
                         
                    // (移除 balloc 内部的 printf 以减少日志混乱)
                    return bnum; // 返回新分配的块号
               }
          }
          brelse(bp);
     }
       
     printf("balloc: 磁盘空间不足\n");
     return 0;
}

/**
 * @brief 释放一个磁盘块（在位图中标记为空闲）
 *
 * @param dev 设备号
 * @param b 要释放的块号
 */
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

     bp = bread(dev, BBLOCK(b, sb)); // 定位到 bitmap 块
     bi = b % BPB;
     m = 1 << (bi % 8);
       
     if((bp->data[bi/8] & m) == 0) {
          panic("bfree: 释放空闲块");
     }
       
     bp->data[bi/8] &= ~m; // 将位清零
     log_write(bp); // 将 bitmap 块的更改写入日志
     brelse(bp);
}

// =================================================================
// 第 3 层: Inode 管理 (分配、缓存、读写)
// =================================================================

/**
 * @brief 在设备 dev 上分配一个 inode
 *
 * 扫描磁盘 inode 块，查找一个类型为 0 (空闲) 的 dinode。
 * @param dev 设备号
 * @param type 新 inode 的类型 (T_FILE, T_DIR, T_DEV)
 * @return 成功则返回内存中的 inode 指针，失败则返回 0
 */
struct inode* ialloc(uint dev, short type) {
     int inum;
     struct buf *bp;
     struct dinode *dip;

     // 遍历所有 inode (从 1 开始，0 保留)
     for(inum = 1; inum < sb.ninodes; inum++){
          bp = bread(dev, IBLOCK(inum, sb)); // 读取 inode 所在的块
          dip = (struct dinode*)bp->data + inum%IPB; // 定位到具体的 dinode
          if(dip->type == 0){     // 找到空闲 inode
               memset(dip, 0, sizeof(*dip));
               dip->type = type;
               log_write(bp);         // *日志操作*: 将 inode 块的更改写入日志
               brelse(bp);
                 
               // 获取新分配 inode 的内存副本
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

/**
 * @brief 将内存中 inode 的更改写回磁盘 (dinode)
 *
 * @param ip 内存中的 inode 指针
 */
void iupdate(struct inode *ip) {
     struct buf *bp;
     struct dinode *dip;

     bp = bread(ip->dev, IBLOCK(ip->inum, sb)); // 读取 inode 所在的块
     dip = (struct dinode*)bp->data + ip->inum%IPB; // 定位
     dip->type = ip->type;
     dip->major = ip->major;
     dip->minor = ip->minor;
     dip->nlink = ip->nlink;
     dip->size = ip->size;
       
     for(int i = 0; i < NDIRECT+1; i++) // 复制数据块地址
          dip->addrs[i] = ip->addrs[i];
             
     log_write(bp); // *日志操作*: 将 inode 块的更改写入日志
     brelse(bp);
}

/**
 * @brief 获取一个内存中的 inode (inode 缓存)
 *
 * 查找 inode 表 (itable)，看 (dev, inum) 是否已在内存中。
 * 如果在，增加引用计数并返回。
 * 如果不在，找一个空槽，初始化它 (ref=1, valid=0)，并返回。
 * @param dev 设备号
 * @param inum inode 编号
 * @return 内存中的 inode 指针
 */
struct inode* iget(uint dev, uint inum) {
     struct inode *ip, *empty;

     // 是否已经在表中缓存？
     empty = 0;
     for(ip = &itable.inode[0]; ip < &itable.inode[50]; ip++){
          if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
               ip->ref++; // 增加引用计数
               return ip; // 命中缓存
          }
          if(empty == 0 && ip->ref == 0)          // 记住第一个空槽位
               empty = ip;
     }

     // 未命中缓存，回收空的 inode 表条目
     if(empty == 0)
          panic("iget: inode 表已满");

     ip = empty;
     ip->dev = dev;
     ip->inum = inum;
     ip->ref = 1; // 引用计数为 1
     ip->valid = 0; // *重要*: 标记为无效，数据尚未从磁盘加载

     return ip;
}

/**
 * @brief 增加对内存中 inode 的引用计数 (复制文件描述符时使用)
 */
struct inode* idup(struct inode *ip) {
     ip->ref++;
     return ip;
}

/**
 * @brief 锁定给定的 inode
 *
 * 在这个简化系统中，"锁定" 意味着 "确保 inode 的数据已从磁盘加载"。
 * 如果 ip->valid == 0, 则从磁盘读取 dinode 并填充内存 inode。
 * @param ip 内存中的 inode 指针
 */
void ilock(struct inode *ip) {
     struct buf *bp;
     struct dinode *dip;

     if(ip == 0 || ip->ref < 1)
          panic("ilock");

     if(ip->valid == 0){ // *缓存未命中*
          // 从磁盘读取 dinode
          bp = bread(ip->dev, IBLOCK(ip->inum, sb));
          dip = (struct dinode*)bp->data + ip->inum%IPB;
          
          // 复制元数据到内存 inode
          ip->type = dip->type;
          ip->major = dip->major;
          ip->minor = dip->minor;
          ip->nlink = dip->nlink;
          ip->size = dip->size;
             
          for(int i = 0; i < NDIRECT+1; i++) // 复制数据块地址
               ip->addrs[i] = dip->addrs[i];
                 
          brelse(bp);
          ip->valid = 1; // 标记为有效（数据已加载）
          
          if(ip->type == 0) // 从磁盘读到了一个空 inode
               panic("ilock: 无类型");
     }
}

/**
 * @brief 解锁给定的 inode (在此简化实现中为空操作)
 */
void iunlock(struct inode *ip) {
     if(ip == 0 || ip->ref < 1)
          panic("iunlock");
}

/**
 * @brief 减少对内存中 inode 的引用 (iput)
 *
 * *关键逻辑*: 当引用计数降为 1 并且链接数 (nlink) 为 0 时，
 * 这意味着这是最后一个指向该 inode 的引用，且文件已被删除。
 * 此时，必须释放 inode 的所有数据块 (itrunc) 并清空 inode (type=0)。
 * @param ip 内存中的 inode 指针
 */
void iput(struct inode *ip) {
     // 检查是否这是最后一个引用，并且文件已无链接（已被删除）
     if(ip->ref == 1 && ip->valid && ip->nlink == 0){
          // inode没有链接，没有其他引用：截断并释放
          ilock(ip); // 确保数据已加载 (虽然在这里主要是为了配对)
             
          itrunc(ip); // 释放所有数据块
          ip->type = 0; // 标记 inode 为空闲
          iupdate(ip); // 将 (type=0) 写回磁盘
          ip->valid = 0; // 标记内存 inode 为无效
             
          iunlock(ip);
     }

     ip->ref--; // 减少引用计数
}

/**
 * @brief 辅助函数：解锁并释放
 */
void iunlockput(struct inode *ip) {
     iunlock(ip);
     iput(ip);
}

// =================================================================
// 第 4 层: 文件内容 I/O (数据块映射)
// =================================================================

/**
 * @brief 块映射 (Block Map)
 *
 * 将文件内的逻辑块号 (bn) 转换为磁盘上的物理块号 (地址)。
 * *按需分配*: 如果请求的块尚未分配，此函数会调用 balloc 分配新块。
 * @param ip Inode
 * @param bn 逻辑块号 (文件内的第 N 块)
 * @return 物理块号 (磁盘地址)，失败则返回 0
 */
static uint bmap(struct inode *ip, uint bn) {
     uint addr, *a;
     struct buf *bp;

     // 1. 直接块 (NDIRECT)
     if(bn < NDIRECT){
          if((addr = ip->addrs[bn]) == 0){ // 该块尚未分配
               addr = balloc(ip->dev); // *按需分配*
               if(addr == 0)
                    return 0; // 磁盘空间不足
               ip->addrs[bn] = addr; // 记录新块的地址
          }
          return addr;
     }
     bn -= NDIRECT; // 计算一级间接块中的索引

     // 2. 一级间接块 (NINDIRECT)
     if(bn < NINDIRECT){
          // 加载间接块，必要时分配
          if((addr = ip->addrs[NDIRECT]) == 0){ // 间接块本身尚未分配
               addr = balloc(ip->dev); // *按需分配*
               if(addr == 0)
                    return 0;
               ip->addrs[NDIRECT] = addr; // 记录间接块的地址
          }
          bp = bread(ip->dev, addr); // 读取间接块
          a = (uint*)bp->data;
          if((addr = a[bn]) == 0){ // 间接块中的数据块地址尚未分配
               addr = balloc(ip->dev); // *按需分配*
               if(addr){
                    a[bn] = addr; // 记录新数据块的地址
                    log_write(bp); // *日志操作*: 间接块已被修改，写入日志
               }
          }
          brelse(bp);
          return addr;
     }

     panic("bmap: 超出范围");
     return 0;
}

/**
 * @brief 截断 inode (释放所有数据块)
 * @param ip Inode
 */
void itrunc(struct inode *ip) {
     int i, j;
     struct buf *bp;
     uint *a;

     // 1. 释放所有直接块
     for(i = 0; i < NDIRECT; i++){
          if(ip->addrs[i]){
               bfree(ip->dev, ip->addrs[i]);
               ip->addrs[i] = 0;
          }
     }

     // 2. 释放所有间接块
     if(ip->addrs[NDIRECT]){
          bp = bread(ip->dev, ip->addrs[NDIRECT]); // 读取间接块
          a = (uint*)bp->data;
          for(j = 0; j < NINDIRECT; j++){ // 遍历间接块中的所有地址
               if(a[j])
                    bfree(ip->dev, a[j]); // 释放数据块
          }
          brelse(bp);
          bfree(ip->dev, ip->addrs[NDIRECT]); // 释放间接块本身
          ip->addrs[NDIRECT] = 0;
     }

     ip->size = 0;
     iupdate(ip); // 将 inode 的更改 (size=0, addrs=0) 写回磁盘
}

/**
 * @brief 从 inode 读取数据 (readi)
 *
 * @param ip Inode
 * @param user_dst 是否复制到用户空间 (简化版中忽略)
 * @param dst 目标内存地址 (内核地址)
 * @param off 文件偏移量
 * @param n 要读取的字节数
 * @return 实际读取的字节数
 */
int readi(struct inode *ip, int user_dst, uint64 dst, uint off, uint n) {
     uint tot, m;
     struct buf *bp;

     // 边界检查
     if(off > ip->size || off + n < off)
          return 0;
     if(off + n > ip->size)
          n = ip->size - off; // 调整为实际可读字节数

     for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
          // 1. 获取物理块号
          uint addr = bmap(ip, off/BSIZE);
          if(addr == 0)
               break; // 块未映射 (不应发生)
          
          // 2. 读取物理块
          bp = bread(ip->dev, addr);
          
          // 3. 计算在此块中要读取多少字节
          m = min(n - tot, BSIZE - off%BSIZE);
             
          // 4. 复制数据 (简化版本：假设 user_dst==0，直接复制到内核地址)
          for(uint i = 0; i < m; i++){
               ((char*)dst)[i] = bp->data[off%BSIZE + i];
          }
             
          brelse(bp);
     }
     return tot; // 返回总共读取的字节数
}

/**
 * @brief 向 inode 写入数据 (writei)
 *
 * @param ip Inode
 * @param user_src 是否从用户空间复制 (简化版中忽略)
 * @param src 源内存地址 (内核地址)
 * @param off 文件偏移量
 * @param n 要写入的字节数
 * @return 实际写入的字节数
 */
int writei(struct inode *ip, int user_src, uint64 src, uint off, uint n) {
     uint tot, m;
     struct buf *bp;

     // 边界检查
     if(off > ip->size || off + n < off)
          return -1;
     if(off + n > MAXFILE*BSIZE) // 检查是否超过最大文件大小
          return -1;

     for(tot=0; tot<n; tot+=m, off+=m, src+=m){
          // 1. 获取物理块号 (如果不存在，bmap 会自动分配)
          uint addr = bmap(ip, off/BSIZE);
          if(addr == 0) {
               printf("writei: bmap返回0，bn=%d\r\n", off/BSIZE);
               break; // 磁盘空间不足
          }
          
          // 2. 读取物理块
          bp = bread(ip->dev, addr);
          
          // 3. 计算在此块中要写入多少字节
          m = min(n - tot, BSIZE - off%BSIZE);
             
          // 4. 复制数据 (简化版本：假设 user_src==0，直接从内核地址复制)
          for(uint i = 0; i < m; i++){
               bp->data[off%BSIZE + i] = ((char*)src)[i];
          }
             
          log_write(bp); // *日志操作*: 数据块已被修改，写入日志
          brelse(bp);
     }

     // 5. 更新 inode 大小 (如果文件变大了)
     if(off > ip->size)
          ip->size = off;

     // 6. 将 inode 的更改 (size) 写回磁盘
     iupdate(ip);

     return tot;
}

// =================================================================
// 第 5 层: 目录与路径名
// =================================================================

/**
 * @brief 比较两个文件名
 */
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
/**
 * @brief 在目录中查找 name 的 inode (优化版)
 *
 * *优化*: 此实现采用 "逐块 I/O" 而不是 "逐条目 I/O"。
 * 它一次读取一整个数据块 (BSIZE)，然后在内存中扫描该块中的所有
 * 目录项 (dirent)，而不是每次只读取一个 dirent。
 *
 * @param dp 目录 inode
 * @param name 要查找的文件名
 * @param poff (可选) 如果找到，设置此指针为条目在目录中的字节偏移量
 * @return 成功则返回*解锁*的 inode，失败则返回 0
 */
struct inode* dirlookup(struct inode *dp, char *name, uint *poff) {
     uint off, inum;
     struct buf *bp;

     if(dp->type != T_DIR)
          panic("dirlookup 不是目录");

  // 每次循环读取一个数据块, 而不是一个 dirent
     for(off = 0; off < dp->size; off += BSIZE){
          // 1. 获取目录的数据块 (bmap 会在需要时分配)
          bp = bread(dp->dev, bmap(dp, off / BSIZE));
          if(bp == 0)
              panic("dirlookup: bmap/bread failed");

          // 2. 在内存中 (bp->data) 扫描这个块中的所有条目
          struct dirent *de = (struct dirent*)bp->data;
          for(int i = 0; i < (BSIZE / sizeof(struct dirent)); i++, de++){
              if(de->inum == 0) // 空槽
                  continue;
               
              if(namecmp(name, de->name) == 0){
                  // 3. 找到条目
                  if(poff)
                      *poff = off + i * sizeof(struct dirent);
                  inum = de->inum;
                 
                  brelse(bp);
                  return iget(dp->dev, inum); // 获取 inode 并返回
              }
          }
          // 4. 未在此块中找到, 释放块
          brelse(bp);
     }

     return 0; // 未找到
}
// -----------------------------------------------------------------


// -----------------------------------------------------------------
// *** 优化的 dirlink ***
// -----------------------------------------------------------------
/**
 * @brief 将新条目 (name, inum) 写入目录 dp (优化版)
 *
 * *优化*: 同样使用 "逐块 I/O" 来查找空槽。
 *
 * @param dp 目录 inode
 * @param name 要创建的文件名
 * @param inum 要链接的 inode 编号
 * @return 成功则返回 0, 失败则返回 -1
 */
int dirlink(struct inode *dp, char *name, uint inum) {
     int off;
     struct dirent de;
     struct inode *ip;
     struct buf *bp;

     // 1. 检查 name 是否已存在 (使用优化的 dirlookup)
     if((ip = dirlookup(dp, name, 0)) != 0){
          iput(ip); // 如果存在，释放 dirlookup 返回的 inode
          return -1;
     }

  // 2. 查找空的 dirent (使用块扫描优化)
  uint empty_off = 0;
     for(off = 0; off < dp->size; off += BSIZE){
          bp = bread(dp->dev, bmap(dp, off / BSIZE));
          if(bp == 0)
              panic("dirlink: bmap/bread failed");
         
          struct dirent *de_scan = (struct dirent*)bp->data;
          for(int i = 0; i < (BSIZE / sizeof(struct dirent)); i++){
              if(de_scan[i].inum == 0) { // 找到空槽
                  empty_off = off + i * sizeof(struct dirent);
                  brelse(bp);
                  goto slot_found; // 跳出两层循环
              }
          }
          brelse(bp);
     }
  // 未在现有块中找到空槽，追加到末尾
  empty_off = dp->size;

slot_found:
  // 3. 增加被链接 inode 的 nlink
  ip = iget(dp->dev, inum); // 获取要链接的 inode
  if(ip == 0)
     panic("dirlink: iget failed");
   
  ilock(ip);
  ip->nlink++; // 增加链接计数
  iupdate(ip); // 将 nlink 更改写回磁盘
  iunlockput(ip); // 释放从 iget() 获得的引用

     // 4. 填充并写入新的 dirent
     memset(&de, 0, sizeof(de));
     de.inum = inum;
     int i;
     for(i = 0; i < DIRSIZ && name[i]; i++) // 复制文件名
          de.name[i] = name[i];
       
     // 5. 使用 writei 在找到的偏移量处写入 (这会更新目录大小)
     if(writei(dp, 0, (uint64)&de, empty_off, sizeof(de)) != sizeof(de))
          return -1;

     return 0;
}
// -----------------------------------------------------------------


/**
 * @brief 路径解析辅助函数
 *
 * 复制路径 (path) 中的下一个元素到 name。
 * 例如：path = "/usr/bin", name = ... -> 复制 "usr" 到 name, 返回 "/bin"
 * @param path 完整路径
 * @param name (出参) 用于存储下一个路径元素
 * @return 剩余的路径，如果已是末尾则返回 0
 */
static char* skipelem(char *path, char *name) {
     char *s;
     int len;

     while(*path == '/') // 跳过前导 '/'
          path++;
     if(*path == 0) // 路径已空
          return 0;
     s = path;
     while(*path != '/' && *path != 0) // 查找下一个 '/' 或末尾
          path++;
     len = path - s;
     if(len >= DIRSIZ) // 截断过长的文件名
          len = DIRSIZ - 1;
     for(int i = 0; i < len; i++) // 复制元素
          name[i] = s[i];
     name[len] = 0;
     while(*path == '/') // 跳过结尾的 '/'
          path++;
     return path; // 返回剩余路径
}

/**
 * @brief 路径解析核心函数 (namex)
 *
 * 遍历路径，逐级查找 inode。
 * @param path 完整路径
 * @param nameiparent 
 *    - 0: 解析完整路径，返回目标 inode (namei)
 *    - 1: 解析到父目录，返回父目录 inode (nameiparent)，
 *         并将最后一个元素复制到 name
 * @param name (出参) 见 nameiparent
 * @return 成功则返回 inode，失败则返回 0
 */
static struct inode* namex(char *path, int nameiparent, char *name) {
     struct inode *ip, *next;

     if(*path == '/')
          ip = iget(1, ROOTINO);     // 从根目录开始 (dev = 1)
     else
          // 简化实现：总是从根目录开始 (不支持当前工作目录)
          ip = iget(1, ROOTINO);

     // 循环解析路径
     while((path = skipelem(path, name)) != 0){
          ilock(ip); // 锁定当前目录 (加载数据)
          if(ip->type != T_DIR){ // 路径中间不是目录
               iunlockput(ip);
               return 0;
          }
          
          // *nameiparent 逻辑*:
          // 如果 (nameiparent=1) 并且这是路径的最后一个元素 (*path == '\0')
          if(nameiparent && *path == '\0'){
               // 停止解析，返回当前 inode (即父目录)
               iunlock(ip);
               return ip;
          }
          
          // (现在调用优化的 dirlookup)
          if((next = dirlookup(ip, name, 0)) == 0){ // 在当前目录中查找
               iunlockput(ip); // 未找到
               return 0;
          }
          
          iunlockput(ip); // 释放当前目录 inode
          ip = next; // 进入下一级目录
     }
     
     // 循环结束 (path 为 0)
     if(nameiparent){ // 如果是 nameiparent, 但路径是 "/" 或 "/foo"
          iput(ip); // (这意味着没有父目录)
          return 0;
     }
     return ip; // 返回最终的 inode
}

/**
 * @brief 路径解析 (namei)
 *
 * 返回路径 path 对应的 inode。
 */
struct inode* namei(char *path) {
     char name[DIRSIZ];
     return namex(path, 0, name); // nameiparent=0
}

/**
 * @brief 路径解析 (nameiparent)
 *
 * 返回路径 path 的 *父目录* inode，并将最后一个元素复制到 name。
 */
struct inode* nameiparent(char *path, char *name) {
     return namex(path, 1, name); // nameiparent=1
}