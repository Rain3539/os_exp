/**
 * 文件系统头文件
 * 包含文件系统相关定义
 */

#ifndef FS_H
#define FS_H

#include "types.h"

// 文件系统常量
#define ROOTINO 1   // 根目录inode编号
#define BSIZE 1024  // 块大小
#define NDIRECT 12  // 直接块数量

// 文件系统超级块
struct superblock {
    uint magic;        // 必须是FSMAGIC
    uint size;         // 文件系统镜像大小(块数)
    uint nblocks;      // 数据块数量
    uint ninodes;      // inode数量
    uint nlog;         // 日志块数量
    uint logstart;     // 第一个日志块的块号
    uint inodestart;   // 第一个inode块的块号
    uint bmapstart;    // 第一个空闲位图块的块号
};

#define FSMAGIC 0x10203040  // 文件系统魔数

#endif /* FS_H */