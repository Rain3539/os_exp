//
// 格式化控制台输出 -- printf, panic
//

#include <stdarg.h>
#include "kernel.h"
#include "memlayout.h"
#include "riscv.h"

volatile int panicking = 0; // 正在打印恐慌消息
volatile int panicked = 0;  // 在恐慌结束时永远旋转

// 避免交错并发printf的锁
static struct {
    struct spinlock lock;  // 打印锁
} pr;

static char digits[] = "0123456789abcdef";  // 十六进制数字字符

// 打印整数
static void
printint(long long xx, int base, int sign)
{
    char buf[20];  // 缓冲区
    int i;
    unsigned long long x;

    if (sign && (sign = (xx < 0)))
        x = -xx;  // 处理负数
    else
        x = xx;

    i = 0;
    do {
        buf[i++] = digits[x % base];  // 转换数字
    } while ((x /= base) != 0);

    if (sign)
        buf[i++] = '-';  // 添加负号

    while (--i >= 0)
        consputc(buf[i]);  // 输出字符
}

// 打印指针
static void
printptr(uint64 x)
{
    int i;
    consputc('0');
    consputc('x');
    for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
        consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);  // 输出十六进制
}

// 新增功能：格式化字符串到缓冲区
// 简化实现，仅支持基本格式
static int
vsnprintf(char* buf, int size, const char* fmt, va_list ap)
{
    int i, n = 0;
    char* s;
    
    for (i = 0; fmt[i] != 0 && n < size - 1; i++) {
        if (fmt[i] != '%') {
            buf[n++] = fmt[i];
            continue;
        }
        
        i++;
        switch (fmt[i]) {
        case 'd': {
            int num = va_arg(ap, int);
            char num_buf[20];
            int j = 0, neg = 0;
            
            if (num < 0) {
                neg = 1;
                num = -num;
            }
            
            do {
                num_buf[j++] = digits[num % 10];
                num /= 10;
            } while (num != 0);
            
            if (neg && n < size - 1) {
                buf[n++] = '-';
            }
            
            while (j > 0 && n < size - 1) {
                buf[n++] = num_buf[--j];
            }
            break;
        }
        case 's':
            s = va_arg(ap, char*);
            if (s == 0) s = "(null)";
            while (*s != 0 && n < size - 1) {
                buf[n++] = *s++;
            }
            break;
        case 'c':
            if (n < size - 1) {
                buf[n++] = (char)va_arg(ap, int);
            }
            break;
        case '%':
            if (n < size - 1) {
                buf[n++] = '%';
            }
            break;
        default:
            if (n < size - 1) {
                buf[n++] = '%';
                buf[n++] = fmt[i];
            }
            break;
        }
    }
    
    buf[n] = '\0';
    return n;
}

// 格式化输出到缓冲区
int
snprintf(char* buf, int size, const char* fmt, ...)
{
    va_list ap;
    int n;
    
    va_start(ap, fmt);
    n = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    
    return n;
}

// 打印到控制台
int
printf(char* fmt, ...)
{
    va_list ap;  // 可变参数列表
    int i, cx, c0, c1, c2;
    char* s;

    if (panicking == 0)
        acquire(&pr.lock);  // 获取打印锁

    va_start(ap, fmt);  // 初始化可变参数
    for (i = 0; (cx = fmt[i] & 0xff) != 0; i++) {
        if (cx != '%') {
            consputc(cx);  // 普通字符直接输出
            continue;
        }
        i++;
        c0 = fmt[i + 0] & 0xff;
        c1 = c2 = 0;
        if (c0) c1 = fmt[i + 1] & 0xff;
        if (c1) c2 = fmt[i + 2] & 0xff;
        if (c0 == 'd') {
            printint(va_arg(ap, int), 10, 1);  // 有符号十进制
        }
        else if (c0 == 'l' && c1 == 'd') {
            printint(va_arg(ap, uint64), 10, 1);  // 长有符号十进制
            i += 1;
        }
        else if (c0 == 'l' && c1 == 'l' && c2 == 'd') {
            printint(va_arg(ap, uint64), 10, 1);  // 长长有符号十进制
            i += 2;
        }
        else if (c0 == 'u') {
            printint(va_arg(ap, uint32), 10, 0);  // 无符号十进制
        }
        else if (c0 == 'l' && c1 == 'u') {
            printint(va_arg(ap, uint64), 10, 0);  // 长无符号十进制
            i += 1;
        }
        else if (c0 == 'l' && c1 == 'l' && c2 == 'u') {
            printint(va_arg(ap, uint64), 10, 0);  // 长长无符号十进制
            i += 2;
        }
        else if (c0 == 'x') {
            printint(va_arg(ap, uint32), 16, 0);  // 十六进制
        }
        else if (c0 == 'l' && c1 == 'x') {
            printint(va_arg(ap, uint64), 16, 0);  // 长十六进制
            i += 1;
        }
        else if (c0 == 'l' && c1 == 'l' && c2 == 'x') {
            printint(va_arg(ap, uint64), 16, 0);  // 长长十六进制
            i += 2;
        }
        else if (c0 == 'p') {
            printptr(va_arg(ap, uint64));  // 指针
        }
        else if (c0 == 'c') {
            consputc(va_arg(ap, uint));  // 字符
        }
        else if (c0 == 's') {
            if ((s = va_arg(ap, char*)) == 0)
                s = "(null)";  // 空指针处理
            for (; *s; s++)
                consputc(*s);  // 输出字符串
        }
        else if (c0 == '%') {
            consputc('%');  // 百分号
        }
        else if (c0 == 0) {
            break;  // 格式字符串结束
        }
        else {
            // 打印未知%序列以引起注意
            consputc('%');
            consputc(c0);
        }

    }
    va_end(ap);  // 结束可变参数处理

    if (panicking == 0)
        release(&pr.lock);  // 释放打印锁

    return 0;
}

// 内核恐慌处理
void
panic(char* s)
{
    panicking = 1;
    printf("panic: ");
    printf("%s\n", s);
    panicked = 1; // 冻结来自其他CPU的uart输出
    for (;;)
        ;  // 无限循环
}

// 打印系统初始化
void
printfinit(void)
{
    initlock(&pr.lock, "pr");  // 初始化打印锁
}

// 注意：自旋锁函数在spinlock.c中实现