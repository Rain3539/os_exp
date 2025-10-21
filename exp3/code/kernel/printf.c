#include <stdio.h>
#include <stdarg.h>
#include <console.h>
#include <stddef.h>

// --- 优化：为十进制转换提供两位数字的查找表 ---
static const char g_two_digits[] =
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899";

/**
 * @brief [重构] 将数字转换为字符串并存入缓冲区
 *
 * 这个函数将一个数字转换为字符串形式，并存储到提供的缓冲区中。
 * 支持十进制和十六进制两种进制。
 *
 * @param out_buf 输出缓冲区
 * @param num 要转换的数字
 * @param base 进制 (10 或 16)
 * @return 写入缓冲区的字符数
 */
static int num_to_str(char *out_buf, long long num, int base) {
    // NULL指针检查
    if (!out_buf) {
        return 0;
    }
    
    char temp_buf[32]; // 临时缓冲区，用于逆序存放数字
    int i = 0;
    const char *digits = "0123456789abcdef";

    // 单独处理0
    if (num == 0) {
        out_buf[0] = '0';
        out_buf[1] = '\0';
        return 1;
    }

    // 对十进制使用查表法优化
    if (base == 10) {
        while (num >= 100) {
            int index = (num % 100) * 2;
            temp_buf[i++] = g_two_digits[index + 1];
            temp_buf[i++] = g_two_digits[index];
            num /= 100;
        }
        if (num < 10) {
            temp_buf[i++] = digits[num];
        } else {
            int index = num * 2;
            temp_buf[i++] = g_two_digits[index + 1];
            temp_buf[i++] = g_two_digits[index];
        }
    } else { // 其他进制使用原始方法 (十六进制)
        unsigned long long n = (unsigned long long)num;
        while (n > 0) {
            temp_buf[i++] = digits[n % base];
            n /= base;
        }
    }

    // 将temp_buf中的结果正序复制到out_buf
    int len = i;
    int k = 0;
    while (--i >= 0) {
        out_buf[k++] = temp_buf[i];
    }
    out_buf[k] = '\0'; // 添加字符串结束符
    return len;
}

/**
 * @brief 安全的strlen实现，带NULL指针检查
 */
static int strlen(const char *s) {
    if (!s) {
        return 0;
    }
    
    int len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}


/**
 * @brief  printf的核心实现，增加返回值和NULL检查
 */
static int vprintf(const char *fmt, va_list ap) {
    if (fmt == NULL) {
        console_puts("(Error: NULL format string)");
        return -1; 
    }

    int count = 0; 
    char num_buf[32];

    while (*fmt) {
        if (*fmt != '%') { 
            console_putc(*fmt);
            count++;
            fmt++;
            continue;
        }

        fmt++; 

        int zero_pad = 0; 
        int width = 0;    

        if (*fmt == '0') { zero_pad = 1; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') { width = width * 10 + (*fmt - '0'); fmt++; }

        switch (*fmt) {
            case 'd': { 
                // [修正] 使用 long long 来接收可变参数，确保能处理64位整数
                long long val = va_arg(ap, long long);
                int is_negative = (val < 0);
                if (is_negative) val = -val;
                int len = num_to_str(num_buf, val, 10);
                if (is_negative) len++;
                int pad_len = (width > len) ? (width - len) : 0;
                char pad_char = zero_pad ? '0' : ' ';
                if (!zero_pad) { for (int i = 0; i < pad_len; i++) console_putc(' '); }
                if (is_negative) console_putc('-');
                if (zero_pad) { for (int i = 0; i < pad_len; i++) console_putc('0'); }
                console_puts(num_buf);
                count += pad_len + len;
                break;
            }
            case 'x': { 
                // [修正] 使用 long long 来接收可变参数，确保能处理64位地址/整数
                long long val = va_arg(ap, long long);
                int len = num_to_str(num_buf, val, 16);
                int pad_len = (width > len) ? (width - len) : 0;
                char pad_char = zero_pad ? '0' : ' ';
                for (int i = 0; i < pad_len; i++) console_putc(pad_char);
                console_puts(num_buf);
                count += pad_len + len;
                break;
            }
            case 's': { 
                const char *s = va_arg(ap, const char *);
                if (s == 0) s = "(null)";
                int len = strlen(s);
                int pad_len = (width > len) ? (width - len) : 0;
                for (int i = 0; i < pad_len; i++) console_putc(' ');
                console_puts(s);
                count += pad_len + len;
                break;
            }
            case 'c': { 
                char c = (char)va_arg(ap, int); // char 在可变参数中会提升为 int
                int pad_len = (width > 1) ? (width - 1) : 0;
                for (int i = 0; i < pad_len; i++) console_putc(' ');
                console_putc(c);
                count += pad_len + 1;
                break;
            }
            case '%': { 
                console_putc('%');
                count++;
                break;
            }
            default: { 
                console_putc('%');
                console_putc(*fmt);
                count += 2;
                break;
            }
        }
        fmt++;
    }
    return count; 
}


int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int count = vprintf(fmt, ap);
    va_end(ap);
    return count;
}


int printf_color(term_color_t color, const char *fmt, ...) {
    int count = 0;
    count += printf("\033[%dm", color);

    va_list ap;
    va_start(ap, fmt);
    count += vprintf(fmt, ap); 
    va_end(ap);

    count += printf("\033[%dm", COLOR_RESET);
    return count;
}