#ifndef PRINTF_H
#define PRINTF_H

#include "types.h"

int printf(char* fmt, ...); // 格式化输出函数
int snprintf(char* buf, int size, const char* fmt, ...);  // snprintf 声明

#endif /* PRINTF_H */