// 文件系统相关的系统调用实现
// 
// 这个文件实现了内核内部使用的文件系统操作函数
// 包含: open, read, write, close, unlink, mkdir 等
//
// 注意：这些是实际的文件系统操作实现，不是陷阱处理的系统调用存根

#include "../def.h"
#include "../type.h"
#include "fs.h"
#include "file.h"
#include "log.h"
#include "../proc/proc.h"
#include "../utils/string.h"

#define NULL 0

// =================================================================
// 内部辅助函数
// =================================================================

// 为当前进程分配文件描述符
static int fdalloc(struct file *f) {
    int fd;
    struct proc *p = myproc();

    for(fd = 0; fd < NOFILE; fd++){
        if(p->ofile[fd] == 0){
            p->ofile[fd] = f;
            return fd;
        }
    }
    return -1;
}

// 创建新文件/目录的核心函数
static struct inode* create(char *path, short type, short major, short minor) {
    struct inode *ip, *dp;
    char name[DIRSIZ];

    if((dp = nameiparent(path, name)) == 0)
        return 0;

    ilock(dp);

    if((ip = dirlookup(dp, name, 0)) != 0){
        iunlockput(dp);
        ilock(ip);
        if(type == T_FILE && ip->type == T_FILE)
            return ip;
        iunlockput(ip);
        return 0;
    }

    if((ip = ialloc(dp->dev, type)) == 0)
        panic("create: ialloc");

    ilock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->nlink = 1;
    iupdate(ip);

    if(type == T_DIR){
        // 创建 . 和 .. 目录项
        dp->nlink++;
        iupdate(dp);
        
        if(dirlink(ip, ".", ip->inum) < 0)
            panic("create dots");
        if(dirlink(ip, "..", dp->inum) < 0)
            panic("create dots");
    }

    if(dirlink(dp, name, ip->inum) < 0)
        panic("create: dirlink");

    iunlockput(dp);
    return ip;
}

// =================================================================
// 系统调用实现 - 文件操作
// =================================================================

// 打开文件
// 
// 参数:
//   path  - 文件路径
//   omode - 打开模式标志:
//           O_RDONLY  - 只读
//           O_WRONLY  - 只写
//           O_RDWR    - 读写
//           O_CREATE  - 如果文件不存在则创建
//           O_TRUNC   - 清空文件内容
// 
// 返回: 文件描述符(>=0) 或 -1(失败)
int open(const char *path, int omode) {
    int fd;
    struct file *f;
    struct inode *ip;

    begin_op();

    if(omode & O_CREATE){
        ip = create((char*)path, T_FILE, 0, 0);
        if(ip == 0){
            end_op();
            return -1;
        }
    } else {
        if((ip = namei((char*)path)) == 0){
            end_op();
            return -1;
        }
        ilock(ip);
        if(ip->type == T_DIR){
            iunlockput(ip);
            end_op();
            return -1;
        }
    }

    // 处理 O_TRUNC 标志
    if(omode & O_TRUNC){
        if(ip->type != T_FILE)
            panic("O_TRUNC on non-file");
        itrunc(ip);
    }

    if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
        if(f)
            fileclose(f);
        iunlockput(ip);
        end_op();
        return -1;
    }

    f->type = FD_INODE;
    f->ip = ip;
    f->off = 0;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

    iunlock(ip);
    end_op();
    return fd;
}

// 从文件读取数据
// 
// 参数:
//   fd  - 文件描述符
//   buf - 目标缓冲区
//   n   - 要读取的字节数
// 
// 返回: 实际读取的字节数 或 -1(失败)
int read(int fd, void *buf, int n) {
    struct file *f;
    struct proc *p = myproc();
    
    if(fd < 0 || fd >= NOFILE || (f=p->ofile[fd]) == 0)
        return -1;
    
    return fileread(f, (uint64)buf, n);
}

// 向文件写入数据
// 
// 参数:
//   fd  - 文件描述符
//   buf - 源缓冲区
//   n   - 要写入的字节数
// 
// 返回: 实际写入的字节数 或 -1(失败)
int write(int fd, const void *buf, int n) {
    struct file *f;
    struct proc *p = myproc();
    
    if(fd < 0 || fd >= NOFILE || (f=p->ofile[fd]) == 0)
        return -1;

    return filewrite(f, (uint64)buf, n);
}

// 关闭文件
// 
// 参数:
//   fd - 文件描述符
// 
// 返回: 0(成功) 或 -1(失败)
int close(int fd) {
    struct file *f;
    struct proc *p = myproc();
    
    if(fd < 0 || fd >= NOFILE || (f=p->ofile[fd]) == 0)
        return -1;

    p->ofile[fd] = 0;
    fileclose(f);
    return 0;
}

// =================================================================
// 系统调用实现 - 文件系统操作
// =================================================================

// 删除文件
// 
// 参数:
//   path - 文件路径
// 
// 返回: 0(成功) 或 -1(失败)
// 
// 注意: 不能删除目录
int unlink(const char *path) {
    struct inode *ip, *dp;
    struct dirent de;
    char name[DIRSIZ];
    uint off;

    begin_op();
    
    if((dp = nameiparent((char*)path, name)) == 0){
        end_op();
        return -1;
    }

    ilock(dp);

    // 查找目录项
    if(dirlookup(dp, name, &off) == 0){
        iunlockput(dp);
        end_op();
        return -1;
    }

    // 获取 inode
    if((ip = dirlookup(dp, name, 0)) == 0)
        panic("unlink: dirlookup failed");
    ilock(ip);

    if(ip->nlink < 1)
        panic("unlink: nlink < 1");
    if(ip->type == T_DIR){
        iunlockput(ip);
        iunlockput(dp);
        end_op();
        return -1;
    }

    // 擦除目录项
    memset(&de, 0, sizeof(de));
    if(writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
        panic("unlink: writei");
    
    iunlockput(dp);

    // 减少链接数
    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);

    end_op();
    return 0;
}

// 创建目录
// 
// 参数:
//   path - 目录路径
// 
// 返回: 0(成功) 或 -1(失败)
int mkdir(const char *path) {
    struct inode *ip;

    begin_op();
    if((ip = create((char*)path, T_DIR, 0, 0)) == 0){
        end_op();
        return -1;
    }
    iunlockput(ip);
    end_op();
    return 0;
}