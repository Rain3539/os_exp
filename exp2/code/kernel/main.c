#include <stdio.h>
#include <console.h>

// 定义一个专门的测试函数
void run_printf_tests(void) {
    printf("--- Running printf tests ---\n\n");

    // --- 基本功能测试 ---
    printf("--- Basic Tests ---\n");
    printf("Integer: %d\n", 42);
    printf("Negative: %d\n", -123);
    printf("Hex: 0x%x\n", 0xABCDEF);
    printf("String: %s\n", "Hello World!");
    printf("Char: %c, Percent: %%\n", 'X');

    // --- 边界情况测试 ---
    printf("\n--- Edge Case Tests ---\n");
    printf("INT_MAX: %d\n", 2147483647);
    printf("INT_MIN: %d\n", -2147483648);
    printf("NULL string: %s\n", (char*)0);
    printf("Empty string: %s\n", "");
    
    // --- 宽度和填充测试 ---
    printf("\n--- Width and Padding Tests ---\n");
    printf("Width (d): [%10d]\n", 123);
    printf("Width (d, negative): [%10d]\n", -123);
    printf("Width (x): [0x%8x]\n", 0xABCD);
    printf("Width (s): [%15s]\n", "right-aligned");
    printf("Width (c): [%5c]\n", 'C');
    
    // --- 填充测试 ---
    printf("\n--- Zero Padding Tests ---\n");
    printf("ZeroPad (d): [%010d]\n", 123);
    printf("ZeroPad (d, negative): [%010d]\n", -123); // 期望输出: [-000000123]
    printf("ZeroPad (x): [0x%08x]\n", 0xABCD);

    printf("\n--- All tests completed ---\n");
}

/**
 * @brief 内核的C语言主函数
 */
void kmain(void) {
    // 1. 清屏
    clear_screen();
    
    // 2. 运行所有printf测试用例
    run_printf_tests();
    
    // 3. 确保所有缓冲的输出都已发送
    console_flush();

    // 4. 内核不应该退出，进入无限循环
    while (1) {
        // Halt
    }
}