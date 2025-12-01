/**
 * 内核参数头文件
 * 包含系统范围的常量和参数
 */

#ifndef PARAM_H
#define PARAM_H

// 系统参数
#define NPROC        64   // 最大进程数
#define NCPU         8    // 最大CPU数
#define NOFILE       16   // 每个进程打开文件数
#define NFILE        100  // 系统打开文件数
#define NINODE       50   // 最大活跃inode数
#define NDEV         10   // 最大主设备号
#define ROOTDEV       1   // 文件系统根磁盘设备号
#define MAXARG       32   // 最大exec参数数
#define MAXOPBLOCKS  10   // 任何文件系统操作写入的最大块数
#define LOGSIZE      (MAXOPBLOCKS*3)  // 磁盘日志中最大数据块数
#define NBUF         (MAXOPBLOCKS*3)  // 磁盘块缓存大小
#define FSSIZE       1000 // 文件系统大小(块数)

#endif /* PARAM_H */