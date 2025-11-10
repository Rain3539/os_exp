#ifndef SYSCALL_FS_H
#define SYSCALL_FS_H

// 文件系统相关系统调用实现
// 
// 注意：这些函数是文件系统的实际实现，使用普通函数名（不带 sys_ 前缀）
// def.h 中的 sys_open, sys_read 等是陷阱处理的存根函数，用于处理用户态系统调用

// 文件操作函数
int open(const char *path, int omode);
int read(int fd, void *buf, int n);
int write(int fd, const void *buf, int n);
int close(int fd);

// 文件系统操作函数
int unlink(const char *path);
int mkdir(const char *path);

#endif