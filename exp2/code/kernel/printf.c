#include <stdio.h>
#include <stdarg.h>
#include <console.h>

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
 * 这个新版本不再直接打印，而是将结果写入`out_buf`。
 * 这样做是为了能先计算出数字字符串的长度，以便处理宽度和填充。
 *
 * @param out_buf 输出缓冲区
 * @param num 要转换的数字
 * @param base 进制 (10 或 16)
 * @return 写入缓冲区的字符数
 */
static int num_to_str(char *out_buf, long long num, int base) {
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
    } else { // 其他进制使用原始方法
        while (num > 0) {
            temp_buf[i++] = digits[num % base];
            num /= base;
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
 * @brief 简单的strlen实现
 */
static int strlen(const char *s) {
    int len = 0;
    while (s && s[len]) {
        len++;
    }
    return len;
}

/**
 * @brief [重构] 内核格式化输出函数，支持宽度和零填充
 *
 * 支持格式:
 * %d, %x, %s, %c, %%
 * 支持宽度: %5d, %8s
 * 支持零填充: %08x
 *
 * @param fmt 格式化字符串
 * @param ... 可变参数
 * @return 总是返回0
 */
int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char num_buf[32]; // 用于存放数字转换结果的缓冲区

    while (*fmt) {
        if (*fmt != '%') {
            console_putc(*fmt);
            fmt++;
            continue;
        }

        fmt++; // 跳过 '%'

        // --- 解析标志和宽度 ---
        int zero_pad = 0;
        int width = 0;

        // 1. 解析标志 (目前只支持'0')
        if (*fmt == '0') {
            zero_pad = 1;
            fmt++;
        }

        // 2. 解析宽度
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        // --- 解析并处理类型 ---
        switch (*fmt) {
            case 'd': {
                long long val = va_arg(ap, int);
                int is_negative = (val < 0);
                if (is_negative) val = -val;

                int len = num_to_str(num_buf, val, 10);
                if (is_negative) len++;

                int pad_len = (width > len) ? (width - len) : 0;
                char pad_char = zero_pad ? '0' : ' ';

                // 非零填充时，先打印空格
                if (!zero_pad) {
                    for (int i = 0; i < pad_len; i++) console_putc(' ');
                }

                if (is_negative) console_putc('-');

                // 零填充时，在符号后面打印0
                if (zero_pad) {
                    for (int i = 0; i < pad_len; i++) console_putc('0');
                }

                console_puts(num_buf);
                break;
            }
            case 'x': {
                long long val = va_arg(ap, int); // 依然按int获取，用long long处理
                int len = num_to_str(num_buf, val, 16);
                int pad_len = (width > len) ? (width - len) : 0;
                char pad_char = zero_pad ? '0' : ' ';

                for (int i = 0; i < pad_len; i++) console_putc(pad_char);
                console_puts(num_buf);
                break;
            }
            case 's': {
                const char *s = va_arg(ap, const char *);
                if (s == 0) s = "(null)";
                int len = strlen(s);
                int pad_len = (width > len) ? (width - len) : 0;

                // 字符串是右对齐，左边补空格
                for (int i = 0; i < pad_len; i++) console_putc(' ');
                console_puts(s);
                break;
            }
            case 'c': {
                char c = (char)va_arg(ap, int);
                int pad_len = (width > 1) ? (width - 1) : 0;
                for (int i = 0; i < pad_len; i++) console_putc(' ');
                console_putc(c);
                break;
            }
            case '%': {
                console_putc('%');
                break;
            }
            default: {
                console_putc('%');
                console_putc(*fmt);
                break;
            }
        }
        fmt++;
    }

    va_end(ap);
    return 0;
}