// 文件系统相关系统调用实现
// 包括：open, read, write, close, unlink, mkdir等

#include "../def.h"
#include "../proc/proc.h"
#include "../fs/fs.h"
#include "../fs/file.h"
#include "../fs/log.h"
#include "../utils/string.h"
#include "syscall.h"

#define NULL 0

// =================================================================
// 辅助函数
// =================================================================

// 字符串比较
static int namecmp(const char *s, const char *t) {
    return strncmp(s, t, DIRSIZ);
}

// 检查目录是否为空
static int isdirempty(struct inode *dp) {
    int off;
    struct dirent de;

    for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
        if(readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
            panic("isdirempty: readi");
        if(de.inum != 0)
            return 0;
    }
    return 1;
}

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
// 内核内部文件操作函数
// =================================================================

// 打开文件
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
        if(ip->type == T_DIR && omode != O_RDONLY){
            iunlockput(ip);
            end_op();
            return -1;
        }
    }

    if(omode & O_TRUNC && ip->type == T_FILE){
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

// 从文件读取
int read(int fd, void *buf, int n) {
    struct file *f;
    struct proc *p = myproc();
    
    if(fd < 0 || fd >= NOFILE || (f=p->ofile[fd]) == 0)
        return -1;
    
    return fileread(f, (uint64)buf, n);
}

// 向文件写入
int write(int fd, const void *buf, int n) {
    struct file *f;
    struct proc *p = myproc();
    
    if(fd < 0 || fd >= NOFILE || (f=p->ofile[fd]) == 0)
        return -1;

    return filewrite(f, (uint64)buf, n);
}

// 关闭文件
int close(int fd) {
    struct file *f;
    struct proc *p = myproc();
    
    if(fd < 0 || fd >= NOFILE || (f=p->ofile[fd]) == 0)
        return -1;

    p->ofile[fd] = 0;
    fileclose(f);
    return 0;
}

// 删除文件
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

    // 不能删除 "." 和 ".."
    if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0){
        iunlockput(dp);
        end_op();
        return -1;
    }

    if((ip = dirlookup(dp, name, &off)) == 0){
        iunlockput(dp);
        end_op();
        return -1;
    }
    ilock(ip);

    if(ip->nlink < 1)
        panic("unlink: nlink < 1");
    if(ip->type == T_DIR && !isdirempty(ip)){
        iunlockput(ip);
        iunlockput(dp);
        end_op();
        return -1;
    }

    memset(&de, 0, sizeof(de));
    if(writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
        panic("unlink: writei");
    if(ip->type == T_DIR){
        dp->nlink--;
        iupdate(dp);
    }
    iunlockput(dp);

    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);

    end_op();
    return 0;
}

// 创建目录
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

// =================================================================
// 系统调用接口
// =================================================================

// 系统调用：读取文件
uint64 sys_read(void) {
    struct proc *p = myproc();
    if(!p) return -1;
    
    int fd = p->trapframe->a0;
    uint64 buf = p->trapframe->a1;
    int n = p->trapframe->a2;
    
    return read(fd, (void*)buf, n);
}

// 系统调用：写入文件
uint64 sys_write(void) {
    struct proc *p = myproc();
    if(!p) return -1;
    
    int fd = p->trapframe->a0;
    uint64 buf = p->trapframe->a1;
    int n = p->trapframe->a2;
    
    return write(fd, (const void*)buf, n);
}

// 系统调用：打开文件
uint64 sys_open(void) {
    struct proc *p = myproc();
    if(!p) return -1;
    
    uint64 path = p->trapframe->a0;
    int omode = p->trapframe->a1;
    
    return open((const char*)path, omode);
}

// 系统调用：关闭文件
uint64 sys_close(void) {
    struct proc *p = myproc();
    if(!p) return -1;
    
    int fd = p->trapframe->a0;
    
    return close(fd);
}

// 系统调用：删除文件
uint64 sys_unlink(void) {
    struct proc *p = myproc();
    if(!p) return -1;
    
    uint64 path = p->trapframe->a0;
    
    return unlink((const char*)path);
}

// 系统调用：创建目录
uint64 sys_mkdir(void) {
    struct proc *p = myproc();
    if(!p) return -1;
    
    uint64 path = p->trapframe->a0;
    
    return mkdir((const char*)path);
}