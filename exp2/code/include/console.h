#ifndef __CONSOLE_H__
#define __CONSOLE_H__

// --- [新增] ANSI颜色代码枚举 ---
typedef enum {
    COLOR_BLACK = 30,
    COLOR_RED = 31,
    COLOR_GREEN = 32,
    COLOR_YELLOW = 33,
    COLOR_BLUE = 34,
    COLOR_MAGENTA = 35,
    COLOR_CYAN = 36,
    COLOR_WHITE = 37,
    COLOR_RESET = 0
} term_color_t;

// 向控制台输出一个字符
void console_putc(char c);

// 向控制台输出一个字符串
void console_puts(const char *s);

// 清除控制台屏幕
void clear_screen(void);

// 强制将控制台缓冲区的内容发送到硬件
void console_flush(void);

// --- [新增] 高级控制台功能 ---
// 将光标移动到指定坐标(x, y)，左上角为(1, 1)
void goto_xy(int x, int y);

// 清除光标所在的整行
void clear_line(void);

#endif // __CONSOLE_H__