#include "../def.h"
#include <stdarg.h>
volatile int panicking = 0; // printing a panic message
volatile int panicked = 0;  // spinning forever at end of a panic
static char digits[] = "0123456789abcdef";

// 简单的整数转字符串函数
static void itoa(long long value, char *str, int base, int sgn) {

  char buf[20];
  int i, neg;
  unsigned long long x;

  neg = 0;
  if (sgn && value < 0) {
    neg = 1;
    x = -value;
  } else {
    x = value;
  }

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);
  if (neg)
    buf[i++] = '-';

  int j = 0;
  while (--i >= 0) {
    str[j++] = buf[i];
  }
  str[j] = '\0';
}

void printint(long long value, int base, int sgn) {
  char tmp[32];
  itoa(value, tmp, base, sgn);
  cons_puts(tmp);
}

// 实现vprintf功能的核心函数
static void vprintf_internal(const char *fmt, va_list ap) {
  const char *p;
  char tmp[32];

  for (p = fmt; *p; p++) {
    if (*p != '%') {
      cons_putc(*p);
      continue;
    }

    // 处理格式化字符
    p++;
    switch (*p) {
    case 'd': // 十进制整数
    case 'i': {
      int value = va_arg(ap, int);
      itoa(value, tmp, 10, 1);
      cons_puts(tmp);
      break;
    }
    case 'x': { // 十六进制整数（小写）
      int value = va_arg(ap, int);
      itoa(value, tmp, 16, 0);
      cons_puts(tmp);
      break;
    }
    case 'X': { // 十六进制整数（大写）
      int value = va_arg(ap, int);
      itoa(value, tmp, 16, 0);
      // 转换为大写
      for (char *c = tmp; *c; c++) {
        if (*c >= 'a' && *c <= 'f') {
          *c = *c - 'a' + 'A';
        }
      }
      cons_puts(tmp);
      break;
    }
    case 'c': {                       // 字符
      char c = (char)va_arg(ap, int); // char被提升为int
      cons_putc(c);
      break;
    }
    case 's': { // 字符串
      const char *s = va_arg(ap, const char *);
      if (s) {
        cons_puts(s);
      } else {
        cons_puts("(null)");
      }
      break;
    }
    case '%': { // 字面量%
      cons_putc('%');
      break;
    }
    default: { // 未知格式，直接输出
      cons_putc('%');
      cons_putc(*p);
      break;
    }
    }
  }
}

void printf(const char *fmt, ...) {
  va_list ap; // 声明va_list变量

  // 初始化可变参数列表，fmt是最后一个固定参数
  va_start(ap, fmt);

  // 调用实际的格式化输出函数
  vprintf_internal(fmt, ap);

  // 清理可变参数列表
  va_end(ap);
}
void panic(char *s) {
  panicking = 1;
  printf("panic: ");
  printf("%s\n", s);
  panicked = 1; // freeze uart output from other CPUs
  for (;;)
    ;
}

void clear_screen(void) {
  // 方案1：发送ANSI转义序列
  cons_puts("\033[2J"); // 清除整个屏幕
  cons_puts("\033[H");  // 光标回到左上角 (1,1)
}

void clear_line(void) {
  // 清除当前行
  cons_puts("\033[K");
}

void goto_xy(int x, int y) {
  // ANSI坐标从1开始，所以需要+1
  printf("\033[%d;%dH", y + 1, x + 1);
}

void set_text_color(const char *color) { printf("\033[3%sm", color); }

void set_background_color(const char *color) { printf("\033[4%sm", color); }

void reset_colors(void) { cons_puts("\033[0m"); }

void printf_color(const char *color, const char *fmt, ...) {

  // 设置颜色
  set_text_color(color);

  // 输出内容 - 简化版本，只支持字符串
  va_list ap; // 声明va_list变量

  // 初始化可变参数列表，fmt是最后一个固定参数
  va_start(ap, fmt);

  // 调用实际的格式化输出函数
  vprintf_internal(fmt, ap);

  // 清理可变参数列表
  va_end(ap);

  // 重置颜色
  reset_colors();
}

