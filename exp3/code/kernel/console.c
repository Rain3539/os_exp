#include <console.h>
#include <stdio.h> // 包含它以方便使用printf构造转义序列

// 声明底层硬件输出函数
// 该函数由底层硬件实现，用于将单个字符输出到串口
extern void uart_putc(char c);

// --- 缓冲区实现 (无变化) ---
#define CONSOLE_BUFFER_SIZE 128 // 定义控制台缓冲区的大小
static char console_buffer[CONSOLE_BUFFER_SIZE]; // 控制台输出缓冲区
static int buffer_idx = 0; // 当前缓冲区的索引，用于记录写入位置

/**
 * @brief 刷新控制台缓冲区
 *
 * 将缓冲区中的内容逐个输出到硬件串口，并清空缓冲区。
 * 如果缓冲区为空，则直接返回。
 */
void console_flush(void) {
    if (buffer_idx == 0) return; // 如果缓冲区为空，直接返回
    for (int i = 0; i < buffer_idx; i++) {
        uart_putc(console_buffer[i]); // 调用底层硬件函数输出字符
    }
    buffer_idx = 0; // 清空缓冲区
}

/**
 * @brief 输出单个字符到控制台
 *
 * 将字符写入缓冲区，当缓冲区满或遇到换行符时，刷新缓冲区。
 *
 * @param c 要输出的字符
 */
void console_putc(char c) {
    console_buffer[buffer_idx++] = c; // 将字符写入缓冲区
    if (buffer_idx >= CONSOLE_BUFFER_SIZE || c == '\n') {
        console_flush(); // 如果缓冲区满或遇到换行符，刷新缓冲区
    }
}

/**
 * @brief 输出字符串到控制台
 *
 * 将字符串中的每个字符依次写入缓冲区，直到字符串结束。
 * 如果字符串为NULL，则直接返回。
 *
 * @param s 要输出的字符串
 */
void console_puts(const char *s) {
    if (!s) return; // 如果字符串为NULL，直接返回
    while (*s != '\0') { // 遍历字符串
        console_putc(*s); // 输出每个字符
        s++;
    }
}

/**
 * @brief 清屏函数
 *
 * 使用ANSI转义序列清除屏幕内容、滚动缓冲区，并将光标移动到屏幕左上角。
 */
void clear_screen(void) {
    console_puts("\033[2J");    // 清除屏幕内容
    console_puts("\033[3J");    // 清除滚动缓冲区（如果终端支持）
    console_puts("\033[H");     // 将光标移动到屏幕左上角
    console_flush();            // 刷新缓冲区，确保命令立即生效
}

// --- [新增] 高级控制台功能实现 ---

/**
 * @brief 将光标移动到指定坐标(x, y)
 *
 * 使用ANSI转义序列 `\033[<L>;<C>H`，其中L是行号, C是列号。
 * 行号和列号从1开始计数。
 *
 * @param x 列坐标 (从1开始)
 * @param y 行坐标 (从1开始)
 */
void goto_xy(int x, int y) {
    // 使用printf来格式化转义序列字符串，这是最简单的方法
    printf("\033[%d;%dH", y, x); // 输出转义序列，将光标移动到指定位置
}

/**
 * @brief 清除光标所在的整行
 *
 * 使用ANSI转义序列 `\033[2K` 清除当前行的内容。
 * 使用 `\r` 将光标移动到行首。
 */
void clear_line(void) {
    console_puts("\033[2K\r"); // 清除当前行并将光标移动到行首
}