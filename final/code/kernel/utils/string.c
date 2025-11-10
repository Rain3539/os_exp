#include "../type.h"
#include "../def.h"

// 基本字符串函数

void* memset(void *dst, int c, uint n) {
    char *cdst = (char *) dst;
    int i;
    for(i = 0; i < n; i++){
        cdst[i] = c;
    }
    return dst;
}

int strlen(const char *s) {
    int n = 0;
    while(s[n])
        n++;
    return n;
}

int strcmp(const char *p, const char *q) {
    while(*p && *p == *q)
        p++, q++;
    return (uchar)*p - (uchar)*q;
}

int strncmp(const char *p, const char *q, uint n) {
    while(n > 0 && *p && *p == *q)
        n--, p++, q++;
    if(n == 0)
        return 0;
    return (uchar)*p - (uchar)*q;
}

char* strcpy(char *dst, const char *src) {
    char *r = dst;
    while((*dst++ = *src++) != 0)
        ;
    return r;
}

char* strncpy(char *dst, const char *src, int n) {
    char *r = dst;
    while(n-- > 0 && (*dst++ = *src++) != 0)
        ;
    while(n-- > 0)
        *dst++ = 0;
    return r;
}

// 简化的整数到字符串转换
static void itoa_helper(int n, char *buf) {
    char temp[16];
    int i = 0;
    
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    // 处理负数
    int is_neg = n < 0;
    if (is_neg) n = -n;

    // 倒序填充
    while (n > 0) {
        temp[i++] = (n % 10) + '0';
        n /= 10;
    }
    if (is_neg) temp[i++] = '-';
    
    // 倒序写回 buf
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

// 简化的 snprintf，支持 %d 和 %s
void snprintf(char *buf, int size, const char *fmt, ...) {
    int *va = (int*)(&fmt + 1);
    char num_buf[16];
    
    char *p = buf;
    const char *f = fmt;
    int arg_idx = 0;
    
    while(*f && (p - buf) < size - 1) {
        if (*f == '%') {
            f++;
            if (*f == 'd') {
                // 整数格式
                itoa_helper(va[arg_idx++], num_buf);
                char *n_ptr = num_buf;
                while (*n_ptr && (p - buf) < size - 1) {
                    *p++ = *n_ptr++;
                }
            } else if (*f == 's') {
                // 字符串格式
                char *str = (char*)va[arg_idx++];
                if(str) {
                    while (*str && (p - buf) < size - 1) {
                        *p++ = *str++;
                    }
                } else {
                    char *null_str = "(null)";
                    while (*null_str && (p - buf) < size - 1) {
                        *p++ = *null_str++;
                    }
                }
            } else {
                // 不支持的格式
                if((p - buf) < size - 1) *p++ = '%';
                if(*f && (p - buf) < size - 1) *p++ = *f;
            }
        } else {
            // 复制普通字符
            *p++ = *f;
        }
        if(*f) f++;
    }
    *p = '\0';
}