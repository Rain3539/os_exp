//
// 控制台输入输出，通过UART实现
// 按行读取输入
// 实现特殊输入字符处理：
//   换行 -- 行结束
//   control-h -- 退格
//   control-u -- 删除行
//   control-d -- 文件结束
//   control-p -- 打印进程列表
//

#include <stdarg.h>
#include "kernel.h"
#include "memlayout.h"
#include "riscv.h"
#include "uart.h"  // 添加 uart.h 包含
#include "printf.h" // 添加 printf.h 包含以使用 snprintf

#define BACKSPACE 0x100     // 退格键ASCII码
#define C(x)  ((x)-'@')     // Control-x 宏定义

// 添加函数声明
void uartwrite(char buf[], int n);
void uartinit(void);
void procdump(void);

//
// 向UART发送一个字符
// 由printf()调用，用于回显输入字符，
// 但不从write()调用
//
void
consputc(int c)
{
    if (c == BACKSPACE) {
        // 如果用户输入退格，用空格覆盖
        uartputc_sync('\b'); uartputc_sync(' '); uartputc_sync('\b');
    }
    else {
        uartputc_sync(c);
    }
}

// 控制台数据结构
struct {
    struct spinlock lock;  // 保护控制台的自旋锁

    // 输入缓冲区
#define INPUT_BUF_SIZE 128
    char buf[INPUT_BUF_SIZE];  // 输入缓冲区
    uint r;  // 读索引
    uint w;  // 写索引
    uint e;  // 编辑索引
} cons;

// 向控制台输出字符串
void
console_puts(const char* s)
{
    for (int i = 0; s[i] != 0; i++) {
        consputc(s[i]);  // 逐个字符输出
    }
}

// 使用ANSI转义序列清屏
void
clear_screen(void)
{
    console_puts("\033[2J\033[H");  // ANSI清屏和光标归位
}

// 新增功能：光标定位
// 使用ANSI转义序列将光标移动到指定位置
// 注意：ANSI转义序列的行和列是从1开始计数的
void
goto_xy(int x, int y)
{
    char buf[32];
    // 构建ANSI转义序列：\033[y;xH
    // 注意：ANSI中先写行号再写列号，且从1开始计数
    snprintf(buf, sizeof(buf), "\033[%d;%dH", y + 1, x + 1);
    console_puts(buf);
}

// 新增功能：清除当前行
// 使用ANSI转义序列清除光标所在行
void
clear_line(void)
{
    console_puts("\033[2K");  // ANSI清除行序列
}

// 新增功能：带颜色的格式化输出
// 使用ANSI转义序列设置颜色，然后调用printf，最后重置颜色
int
printf_color(int color, char* fmt, ...)
{
    va_list ap;
    int result;
    
    // 设置颜色 - 前景色为30+颜色码，背景色为40+颜色码
    // 这里我们只设置前景色，背景色保持默认
    char color_seq[16];
    if (color >= 0 && color <= 7) {
        snprintf(color_seq, sizeof(color_seq), "\033[3%dm", color);
    } else {
        // 默认颜色
        snprintf(color_seq, sizeof(color_seq), "\033[39m");
    }
    console_puts(color_seq);
    
    // 调用原始的printf进行格式化输出
    va_start(ap, fmt);
    result = printf(fmt, ap);
    va_end(ap);
    
    // 重置颜色
    console_puts("\033[0m");
    
    return result;
}

//
// 用户write()到控制台的实现
//
int
consolewrite(int user_src, uint64 src, int n)
{
    char buf[32];  // 临时缓冲区
    int i = 0;

    while (i < n) {
        int nn = sizeof(buf);
        if (nn > n - i)
            nn = n - i;
        if (either_copyin(buf, user_src, src + i, nn) == -1)
            break;
        uartwrite(buf, nn);  // 通过UART写入
        i += nn;
    }

    return i;  // 返回实际写入的字节数
}

//
// 用户read()从控制台的实现
// 复制(最多)一整行输入到dst
// user_dist指示dst是用户地址还是内核地址
//
int
consoleread(int user_dst, uint64 dst, int n)
{
    uint target;
    int c;
    char cbuf;

    target = n;
    acquire(&cons.lock);  // 获取控制台锁
    
    // 如果没有输入可用，直接返回0（简化实现，不阻塞）
    if (cons.r == cons.w) {
        release(&cons.lock);
        return 0;  // 没有数据可读，返回0
    }
    
    while (n > 0) {
        // 检查是否有输入可用
        if (cons.r == cons.w) {
            // 没有更多输入，返回已读取的字节数
            break;
        }

        c = cons.buf[cons.r++ % INPUT_BUF_SIZE];  // 从缓冲区读取字符

        if (c == C('D')) {  // 文件结束符
            if (n < target) {
                // 保存^D供下次使用，确保调用者得到0字节结果
                cons.r--;
            }
            break;
        }

        // 将输入字节复制到用户空间缓冲区
        cbuf = c;
        if (either_copyout(user_dst, dst, &cbuf, 1) == -1)
            break;

        dst++;
        --n;

        if (c == '\n') {
            // 整行已到达，返回到用户级read()
            break;
        }
    }
    release(&cons.lock);  // 释放控制台锁

    return target - n;  // 返回实际读取的字节数
}

//
// 控制台输入中断处理程序
// uartintr()为输入字符调用此函数
// 进行擦除/删除处理，追加到cons.buf，
// 如果整行已到达，唤醒consoleread()
//
void
consoleintr(int c)
{
    acquire(&cons.lock);  // 获取控制台锁

    switch (c) {
    case C('P'):  // 打印进程列表
        procdump();
        break;
    case C('U'):  // 删除行
        while (cons.e != cons.w &&
            cons.buf[(cons.e - 1) % INPUT_BUF_SIZE] != '\n') {
            cons.e--;
            consputc(BACKSPACE);  // 回显退格
        }
        break;
    case C('H'): // 退格
    case '\x7f': // 删除键
        if (cons.e != cons.w) {
            cons.e--;
            consputc(BACKSPACE);  // 回显退格
        }
        break;
    default:
        if (c != 0 && cons.e - cons.r < INPUT_BUF_SIZE) {
            c = (c == '\r') ? '\n' : c;  // 回车转换为换行

            // 回显给用户
            consputc(c);

            // 存储供consoleread()使用
            cons.buf[cons.e++ % INPUT_BUF_SIZE] = c;

            if (c == '\n' || c == C('D') || cons.e - cons.r == INPUT_BUF_SIZE) {
                // 如果整行(或文件结束)已到达，唤醒consoleread()
                cons.w = cons.e;
                wakeup(&cons.r);
            }
        }
        break;
    }

    release(&cons.lock);  // 释放控制台锁
}

// 控制台初始化
void
consoleinit(void)
{
    initlock(&cons.lock, "cons");  // 初始化控制台锁

    uartinit();  // 初始化UART

    // 将读写系统调用连接到consoleread和consolewrite
    devsw[CONSOLE].read = consoleread;
    devsw[CONSOLE].write = consolewrite;
}

// 本实验的简化实现
void procdump(void) {
    // 简化实现：什么都不做
}

// 全局设备开关表
struct devsw devsw[NDEV];