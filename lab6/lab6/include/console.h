#ifndef CONSOLE_H
#define CONSOLE_H

void consoleinit(void);  // 控制台初始化
void clear_screen(void);  // 清屏

// 扩展功能声明
void goto_xy(int x, int y);  // 光标定位
void clear_line(void);       // 清除当前行
int printf_color(int color, char* fmt, ...);  // 颜色输出

// 颜色定义
enum console_colors {
    COLOR_BLACK = 0,
    COLOR_RED = 1,
    COLOR_GREEN = 2,
    COLOR_YELLOW = 3,
    COLOR_BLUE = 4,
    COLOR_MAGENTA = 5,
    COLOR_CYAN = 6,
    COLOR_WHITE = 7,
    COLOR_DEFAULT = 9
};

#endif /* CONSOLE_H */