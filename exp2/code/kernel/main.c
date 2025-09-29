#include <stdio.h>
#include <console.h>
#include <stddef.h>


// 简单的延时函数，用于在演示中暂停，方便观察
void delay(int cycles) {
    volatile int i = cycles;
    while (i > 0) {
        i--;
    }
}
// 基础格式化功能测试
void test_basic_formatting() {
    printf("\n--- 2. 基础格式化功能测试 ---\n");
    printf("整数: %d, 负数: %d, 零: %d\n", 123, -456, 0);
    printf("十六进制: 0x%x, 0x%x\n", 0xDEADBEEF, 12345);
    printf("字符串: \"%s\", 字符: '%c', 百分号: '%%'\n", "Hello OS", 'A');
    printf("宽度: [%8d], [%-8d]\n", 123, 123); // (注: 暂未实现左对齐)
    printf("零填充: [%08d], [%08x]\n", 123, 0xABCD);
    printf("负数零填充: [%08d]\n", -123);
}
// 边界条件处理测试
void test_edge_cases() {
    printf("\n--- 3. 边界条件处理测试 ---\n");
    printf("INT_MAX: %d\n", 2147483647);
    printf("INT_MIN: %d\n", -2147483648);
    // 重新启用这些被注释掉的测试
    printf("空字符串: \"%s\"\n", "");
    printf("NULL字符串参数 (%%s): \"%s\"\n", (char*)NULL);
}
// 错误恢复测试
void test_error_recovery() {
    printf("\n--- 4. 错误恢复测试 ---\n");
    printf("测试1: NULL格式字符串...\n");
    int result = printf(NULL);
    printf("\nprintf(NULL) 返回值: %d (预期为 -1)\n", result);
    
    printf("\n测试2: 未知格式化符号 (%%q)...\n");
    printf("输出 -> %q <-\n", 123); // 123参数将被忽略
}
// 高级控制台功能测试
void test_console_features() {
    printf("\n--- 1. 高级控制台功能测试 (光标/颜色/清屏) ---\n");
    printf("即将开始演示... (3秒后)\n");
    delay(1000000000);

    // 步骤1: 清屏并验证
    clear_screen();
    printf("屏幕已清除，您现在应该看到一个空白屏幕 (暂停3秒)...\n");
    console_flush(); // 强制刷新，确保上面的提示语立即显示
    delay(1000000000);

    // // 步骤2: 再次清屏，然后开始绘制
    // clear_screen(); // 真正开始演示前的最后一次清屏
    // delay(4000000000); 

    // 步骤3: 光标定位和颜色输出
    goto_xy(10, 5);
    printf_color(COLOR_YELLOW, "第一站: (10, 5)");
    console_flush(); // 刷新以确保立即显示
    delay(400000000);
    
    goto_xy(25, 10);
    printf_color(COLOR_CYAN, "第二站: (25, 10)");
    console_flush(); // 刷新以确保立即显示
    delay(400000000);

    goto_xy(5, 15);
    printf_color(COLOR_MAGENTA, "第三站: (5, 15), 这一行即将被清除...");
    console_flush(); // 刷新以确保立即显示
    delay(600000000);

    // 步骤4: 清除行
    goto_xy(1, 15);
    clear_line();
    printf_color(COLOR_GREEN, "行已清除!");
    console_flush(); // 刷新以确保立即显示
    delay(400000000);

    // 步骤5: 恢复光标到屏幕末尾
    goto_xy(1, 20);
    printf("演示结束。\n");
}

// 性能测试 - 大量输出
void test_performance() {
    // [修改] 将标题中的序号改为5
    printf("\n--- 5. 性能测试 (大量输出) ---\n");
    printf("将在3秒后开始连续打印50行...\n");
    delay(800000000);

    for (int i = 1; i <= 50; i++) {
        printf("Line %02d/%d: This is a test of the console's high-volume output capability. The quick brown fox jumps over the lazy dog. 1234567890.\n", i, 50);
    }
    printf("大量输出测试完成。\n");
}

// [修改] 主测试函数，调整了调用顺序
void run_all_tests() {
    test_console_features();
    test_basic_formatting();
    test_edge_cases();
    test_error_recovery();
    test_performance(); 
    printf("\n\n======== 所有测试执行完毕! ========\n");
}

void kmain(void) {

    printf("======== RISC-V OS 测试套件 v1.0 ========\n");
    run_all_tests();
    console_flush(); // 确保所有缓冲内容都已输出
    while (1) {}
}