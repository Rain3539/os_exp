#include "../def.h"

//
// 具体的系统调用实现函数
// 注意: 当前所有函数都只是“存根”(stub)实现。
// 它们打印一条消息表示自己被调用，然后返回一个固定的值。
// 在一个完整的操作系统中，这里将包含每个系统调用的完整逻辑。
//

// 系统调用：进程退出
uint64 sys_exit(void) {
    printf("sys_exit called\n");
    // TODO: 实现真正的进程退出逻辑，例如：清理资源、改变进程状态为僵尸、唤醒父进程。
    return 0;
}

// 系统调用：获取当前进程ID
uint64 sys_getpid(void) {
    printf("sys_getpid called\n");
    // TODO: 返回当前正在执行进程的真实ID。
    return 1;
}

// 系统调用：创建一个新进程（克隆当前进程）
uint64 sys_fork(void) {
    printf("sys_fork called\n");
    // TODO: 实现进程创建逻辑，包括复制父进程的内存空间、文件描述符等。
    return 2;
}

// 系统调用：等待子进程退出
uint64 sys_wait(void) {
    printf("sys_wait called\n");
    // TODO: 实现父进程等待子进程退出的逻辑，并回收子进程资源。
    return 3;
}

// 系统调用：从文件读取数据
uint64 sys_read(void) {
    printf("sys_read called\n");
    // TODO: 从指定的文件描述符读取数据到用户提供的缓冲区。
    return 4;
}

// 系统调用：向文件写入数据
uint64 sys_write(void) {
    printf("sys_write called\n");
    // TODO: 将用户提供的缓冲区中的数据写入到指定的文件描述符。
    return 5;
}

// 系统调用：打开一个文件
uint64 sys_open(void) {
    printf("sys_open called\n");
    // TODO: 根据路径名打开一个文件，并返回一个新的文件描述符。
    return 6;
}

// 系统调用：关闭一个文件
uint64 sys_close(void) {
    printf("sys_close called\n");
    // TODO: 关闭一个由文件描述符指定的文件。
    return 7;
}

// 系统调用：执行一个新的程序
uint64 sys_exec(void) {
    printf("sys_exec called\n");
    // TODO: 在当前进程的上下文中加载并执行一个新的程序。
    return 8;
}

// 系统调用：增加程序的堆内存空间
uint64 sys_sbrk(void) {
    printf("sys_sbrk called\n");
    // TODO: 调整进程的数据段大小，用于动态内存分配。
    return 9;
}