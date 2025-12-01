/**
 * UART驱动头文件
 * UART通信函数声明
 */

#ifndef UART_H
#define UART_H

/**
 * 初始化UART硬件
 * 在使用其他UART函数前必须调用
 */
void uart_init(void);
void uartinit(void); // 添加 uartinit 声明

/**
 * 通过UART发送单个字符
 * @param c 要发送的字符
 */
void uart_putc(char c);

/**
 * 通过UART发送以空字符结尾的字符串
 * @param s 要发送的字符串指针
 */
void uart_puts(char *s);

/**
 * 通过UART发送数据缓冲区
 * @param buf 要发送的缓冲区
 * @param n 要发送的字节数
 */
void uartwrite(char buf[], int n); // 添加 uartwrite 声明

#endif /* UART_H */