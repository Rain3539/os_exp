/**
 * RISC-V OS链接器脚本
 * 定义内核的内存布局和符号位置
 * 此脚本告诉链接器如何在内存中排列代码和数据
 */

OUTPUT_ARCH("riscv")    /* 目标架构是RISC-V */
ENTRY(_entry)           /* 入口点符号 - 执行的第一段代码 */

/* RISC-V ABI的全局指针 - 用于小数据访问优化 */
__global_pointer$ = . + 0x800;

SECTIONS
{
    /* 设置起始地址 - QEMU在内核0x80000000处加载 */
    . = 0x80000000;

    /* 文本段 - 包含可执行代码 */
    .text : {
        *(.text .text.*)     /* 包含所有目标文件的所有文本段 */
        . = ALIGN(0x1000);   /* 对齐到4KB页边界 */
        PROVIDE(etext = .);  /* 为C代码定义文本结束符号 */
    }

    /* 只读数据段 - 包含常量和字符串 */
    .rodata : {
        . = ALIGN(16);           /* 对齐到16字节边界 */
        *(.srodata .srodata.*);  /* 小只读数据(RISC-V特定) */
        . = ALIGN(16);
        *(.rodata .rodata.*);    /* 常规只读数据 */
    }

    /* 数据段 - 包含已初始化的全局变量 */
    .data : {
        . = ALIGN(16);
        *(.sdata .sdata.*);      /* 小数据(RISC-V特定) */
        . = ALIGN(16);
        *(.data .data.*);        /* 常规初始化数据 */
    }

    /* BSS段 - 包含未初始化的全局变量 */
    .bss : {
        . = ALIGN(16);
        PROVIDE(_sbss = .);      /* BSS开始 - 由entry.S使用 */
        *(.sbss .sbss.*);        /* 小BSS(RISC-V特定) */
        *(.bss .bss.*);          /* 常规BSS */
        . = ALIGN(16);
        PROVIDE(_ebss = .);      /* BSS结束 - 由entry.S使用 */
    }

    /* 所有段的结束标记 */
    PROVIDE(end = .);

    /* 栈段 - 函数调用的运行时栈 */
    .stack : {
        . = ALIGN(0x1000);       /* 对齐到4KB页边界 */
        PROVIDE(_stack_top = .); /* 栈从这里向下增长 */
        . += 0x1000;             /* 为栈分配4KB */
        PROVIDE(_stack_bottom = .); /* 栈空间底部 */
    }
}