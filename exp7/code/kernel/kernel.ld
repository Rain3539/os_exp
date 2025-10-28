/* 最小RISC-V操作系统链接脚本 */

OUTPUT_ARCH( "riscv" )
ENTRY( _start )

SECTIONS
{
    /* 设置起始地址 - QEMU RISC-V virt机器的内核加载地址 */
    . = 0x80000000;

    /* 代码段 - 包含所有可执行代码 */
    .text : {
        *(.text .text.*)
        . = ALIGN(4096);        /* 4KB页对齐 */
        PROVIDE(etext = .);     /* 代码段结束标记 */
    }

    /* 只读数据段 - 字符串常量等 */
    .rodata : {
        . = ALIGN(16);          /* 16字节对齐优化访问 */
        *(.rodata .rodata.*)
        . = ALIGN(16);
    }

    /* 已初始化数据段 - 有初值的全局变量 */
    .data : {
        . = ALIGN(16);
        *(.sdata .sdata.*)      /* RISC-V小数据段 */
        *(.data .data.*)
        . = ALIGN(16);
        PROVIDE(edata = .);     /* 数据段结束标记 */
    }

    /* 未初始化数据段 - 需要清零的全局变量 */
    .bss : {
        . = ALIGN(16);
        PROVIDE(__bss_start = .);   /* BSS段开始标记 */
        *(.sbss .sbss.*)            /* RISC-V小BSS段 */
        *(.bss .bss.*)
        . = ALIGN(16);
        PROVIDE(__bss_end = .);     /* BSS段结束标记 */
    }

    /* 内核结束标记 - 用于内存管理 */
    PROVIDE(end = .);

} 