#ifndef STRING_H
#define STRING_H

#include "../type.h"

// 字符串操作函数
void* memset(void *dst, int c, uint n);
int strlen(const char *s);
int strcmp(const char *p, const char *q);
int strncmp(const char *p, const char *q, uint n);
char* strcpy(char *dst, const char *src);
char* strncpy(char *dst, const char *src, int n);
void snprintf(char *buf, int size, const char *fmt, ...);

#endif