#include "def.h"
#include "type.h"
#include "utils/console.h"
/* 最小RISC-V操作系统主函数
 * 验证启动流程是否正确工作
 */

// 测试函数
void test_printf_basic();
void test_printf_edge_cases();
void console_demo(void);
void progress_bar_demo(void);
void test_physical_memory(void);
void test_pagetable(void);
void test_timer_interrupt(void);
void test_interrupt_overhead(void);

void system_shutdown(void);
void delay_seconds(int seconds);

void main(void) {
  printf("My RISC-V OS Starting...\r\n");

  kinit();
  kvminit();
  kvminithart();
  trapinithart();



  //中断功能测试
  test_timer_interrupt();
  printf("Timer interrupt test passed!\n");

  //性能测试
  test_interrupt_overhead();
  printf("Interrupt overhead test passed!\n");

  // test_breakpoint();
  // printf("Breakpoint test passed!\n");

  //异常测试
  test_exception();
  printf("Exception test passed!\n");
  printf("All tests passed!\n");
  printf("System initialization complete!\r\n");
  printf("Entering main loop...\r\n");
  while (1) {
  }
}




void delay_seconds(int seconds) {
  // 简单的忙等待延时（不精确，但可用）
  volatile int count = 0;
  // 假设每次循环大约1微秒，需要调整这个系数
  int loops_per_second = 250000000; // 根据实际情况调整
  printf("Working...\r\n");
  for (int i = 0; i < seconds; i++) {
    for (int j = 0; j < loops_per_second; j++) {
      count++; // 防止编译器优化
    }
    printf("%d...", seconds - i - 1);
  }
}

void system_shutdown(void) {
  printf("System shutting down...\n");

  // QEMU virt机器关机方法
  volatile uint32 *test_finisher = (volatile uint32 *)0x100000;
  *test_finisher = 0x5555; // 正常关机

  // 如果上面不工作，尝试这个
  asm volatile("wfi"); // 等待中断（进入低功耗）
  while (1)
    ; // 无限循环
}
void test_printf_basic() {
  printf("Testing integer: %d\n", 42);
  printf("Testing negative: %d\n", -123);
  printf("Testing zero: %d\n", 0);
  printf("Testing hex: 0x%x\n", 0xABC);
  printf("Testing string: %s\n", "Hello");
  printf("Testing char: %c\n", 'X');
  printf("Testing percent: %%\n");
}
void test_printf_edge_cases() {
  printf("INT_MAX: %d\n", 2147483647);
  printf("INT_MIN: %d\n", -2147483648);
  printf("NULL string: %s\n", (char *)0);
  printf("Empty string: %s\n", "");

}

// 综合演示函数
void console_demo(void) {
  clear_screen();

  // 标题
  goto_xy(20, 2);
  printf_color(ANSI_COLOR_CYAN, "=== RISC-V OS Console Demo ===");

  // 彩色输出演示
  goto_xy(5, 4);
  printf_color(ANSI_COLOR_RED, "Red Text");

  goto_xy(5, 5);
  printf_color(ANSI_COLOR_GREEN, "Green Text");

  goto_xy(5, 6);
  printf_color(ANSI_COLOR_YELLOW, "Yellow Text");

  goto_xy(5, 7);
  printf_color(ANSI_COLOR_BLUE, "Blue Text");

  // 光标移动演示
  goto_xy(30, 4);
  printf("Cursor moved to (30,4)");

  goto_xy(30, 6);
  printf("Drawing a box:");

  // 画一个简单的框
  goto_xy(30, 8);
  printf("+----------+");
  goto_xy(30, 9);
  printf("|   Box    |");
  goto_xy(30, 10);
  printf("+----------+");

  // 底部信息
  goto_xy(10, 11);
  printf_color(ANSI_COLOR_MAGENTA, "Console functions working!");

  goto_xy(10, 12);
  printf_color(ANSI_COLOR_RED, "123%d\n", 111);
  printf("Press any key to continue...\n");
}

void progress_bar_demo(void) {
  clear_screen();
  goto_xy(10, 5);
  cons_puts("Progress Bar Demo:");

  for (int i = 0; i <= 20; i++) {
    goto_xy(10, 7);
    cons_puts("Progress: [");

    // 绘制进度条
    for (int j = 0; j < 20; j++) {
      if (j < i) {
        uart_putc('=');
      } else {
        uart_putc(' ');
      }
    }
    cons_puts("] ");
    printf("%d%%", (i * 100) / 20);

    // 简单延时
    volatile int delay = 0;
    for (int k = 0; k < 1000000; k++) {
      delay++;
    }
  }

  goto_xy(10, 9);
  printf_color(ANSI_COLOR_GREEN, "Complete!");
}


void test_physical_memory(void) {
  // 测试基本分配和释放
  void *page1 = kalloc();
  void *page2 =kalloc();
  assert(page1 != page2);
  assert(((uint64)page1 & 0xFFF) == 0);

  // 测试数据写入
  *(int*)page1 = 0x12345678;
  assert(*(int*)page1 == 0x12345678);

  // 测试释放和重新分配
  kfree(page1);
  void *page3 = kalloc();
  // page3可能等于page1（取决于分配策略）

  kfree(page2);
  kfree(page3);

  printf("Physical memory test-------------------------- passed!\n");

}

void test_pagetable(void) {
// 页对齐检查
pagetable_t pt = create_pagetable();
// 测试基本映射
uint64 va = 0x10000;
uint64 pa = (uint64)kalloc();
assert(mappages(pt, va,4096, pa, PTE_R | PTE_W) == 0);

// 测试地址转换
uint64 *pte = (uint64 *)walk(pt, va,0);
assert(pte != 0 && (*pte & PTE_V));
// printf("*pte: %x\n", PTE2PA(*pte));
// printf("pa: %x\n", pa);

assert(PTE2PA(*pte) == pa);

// 测试权限位
assert(*pte & PTE_R);
assert(*pte & PTE_W);
assert(!(*pte & PTE_X));

printf("Pagetable test-------------------------------- passed!\n");

}