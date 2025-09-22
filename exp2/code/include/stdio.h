#ifndef __STDIO_H__
#define __STDIO_H__

// 包含console.h以获取颜色枚举的定义
#include <console.h>

// 内核格式化输出函数
int printf(const char *fmt, ...);

// [新增] 内核带颜色的格式化输出函数
int printf_color(term_color_t color, const char *fmt, ...);

#endif // __STDIO_H__