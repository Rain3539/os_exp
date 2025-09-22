#ifndef __CONSOLE_H__
#define __CONSOLE_H__

// 向控制台输出一个字符
void console_putc(char c);

// 向控制台输出一个字符串
void console_puts(const char *s);

// 清除控制台屏幕
void clear_screen(void);

// 强制将控制台缓冲区的内容发送到硬件
void console_flush(void);

#endif // __CONSOLE_H__