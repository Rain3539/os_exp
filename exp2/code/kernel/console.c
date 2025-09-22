#include <console.h>

// 声明底层硬件输出函数
extern void uart_putc(char c);

// --- 优化：引入控制台缓冲区 ---
#define CONSOLE_BUFFER_SIZE 128
static char console_buffer[CONSOLE_BUFFER_SIZE];
static int buffer_idx = 0;

/**
 * @brief [修改] 强制将控制台缓冲区的内容发送到硬件
 *
 * 这个函数会将缓冲区内所有字符一次性发送给UART驱动，
 * 然后重置缓冲区索引。
 */
void console_flush(void) {
    if (buffer_idx == 0) {
        return; // 缓冲区为空，无需操作
    }
    for (int i = 0; i < buffer_idx; i++) {
        uart_putc(console_buffer[i]);
    }
    buffer_idx = 0; // 重置缓冲区
}

/**
 * @brief [修改] 向控制台输出一个字符 (带缓冲)
 *
 * 字符首先被写入一个内部缓冲区。当缓冲区满，或者
 * 遇到换行符时，缓冲区内容会被自动“刷出”到硬件。
 * 这样做可以显著减少对底层uart_putc的调用次数，提高性能。
 *
 * @param c 要输出的字符
 */
void console_putc(char c) {
    // 将字符存入缓冲区
    console_buffer[buffer_idx++] = c;

    // 如果缓冲区满了，或者遇到换行符，则刷新缓冲区
    if (buffer_idx >= CONSOLE_BUFFER_SIZE || c == '\n') {
        console_flush();
    }
}

/**
 * @brief 向控制台输出一个字符串.
 *
 * (此函数无需修改，它会自动受益于缓冲的console_putc)
 * @param s 要输出的字符串
 */
void console_puts(const char *s) {
    if (!s) {
        return;
    }
    while (*s != '\0') {
        console_putc(*s);
        s++;
    }
}

/**
 * @brief [修改] 使用ANSI转义序列清屏
 *
 * 在发送完清屏指令后，立即刷新缓冲区以确保指令被执行。
 */
void clear_screen(void) {
    console_puts("\033[2J\033[H");
    console_flush(); // 确保清屏指令立即生效
}