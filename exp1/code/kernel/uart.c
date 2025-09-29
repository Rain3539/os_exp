/* kernel/uart.c - Minimal UART driver for QEMU's virt machine */
#include <stddef.h>

// QEMU中virt机器的UART设备内存映射地址
#define UART_BASE 0x10000000L

// UART寄存器偏移量
#define RHR 0   // 接收保持寄存器（读操作）
#define THR 0   // 发送保持寄存器（写操作） 
#define LSR 5   // 线路状态寄存器

// LSR寄存器的位定义   LSR 寄存器的第5位表示发送器是否准备好接收新数据
#define LSR_TX_IDLE (1 << 5) 

/**
 * @brief 向UART发送一个字符
 * @param c 要发送的字符
 */
void uart_putc(char c) {
    // 等待，直到发送保持寄存器(THR)为空
    // 我们通过检查线路状态寄存器(LSR)的第5位(TX_IDLE)来实现
    // `volatile`关键字确保编译器不会优化掉这个读操作
    while ((*(volatile unsigned char*)(UART_BASE + LSR) & LSR_TX_IDLE) == 0);

    // 将字符写入发送保持寄存器(THR)
    *(volatile unsigned char*)(UART_BASE + THR) = c;
}

/**
 * @brief 向UART发送一个以null结尾的字符串
 * @param s 要发送的字符串
 */
void uart_puts(char *s) {
    if (s == NULL) return;
    while (*s != '\0') {
        uart_putc(*s);
        s++;
    }
}