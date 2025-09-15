/* kernel/main.c - The C entry point for the OS */

// 声明我们在uart.c中定义的函数
void uart_puts(char *s);

/**
 * @brief 内核的C语言主函数
 *
 * 这是在entry.S完成底层设置后调用的第一个C函数。
 */
void kmain(void) {
    // 使用我们的UART驱动打印一条欢迎信息
    uart_puts("Hello OS \n");

    // 内核不应该退出，所以我们进入一个无限循环
    while (1) {
        // 在这里保持CPU忙碌，防止它执行内存中的无效指令
    }
}