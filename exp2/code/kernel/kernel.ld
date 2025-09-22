/* kernel/kernel.ld - Minimal Linker Script for RISC-V OS */

/* 1. 指定输出架构 */
OUTPUT_ARCH(riscv)
/* 2. 指定程序入口点，必须与entry.S中的标签一致 */
ENTRY(_start)

/* 3. 定义内核加载的基地址 */
BASE_ADDRESS = 0x80000000;

SECTIONS
{
    /* 定位器(.)从基地址开始 */
    . = BASE_ADDRESS;

    /* 4. 代码段 (.text) */
    .text : {
        /* 将所有输入文件的代码段放在这里 */
        *(.text.entry) /* 确保_start的代码在最前面 */
        *(.text .text.*)
    }

    /* 5. 只读数据段 (.rodata) */
    .rodata : {
        *(.rodata .rodata.*)
    }

    /* 6. 已初始化数据段 (.data) */
    .data : {
        *(.data .data.*)
    }
    
    /* 7. BSS段 (未初始化数据) */
    /* 我们需要定义符号来帮助汇编代码清零BSS段 */
    . = ALIGN(8); /* 8字节对齐 */
    PROVIDE(sbss = .); /* 定义BSS段的起始地址符号 */
    .bss : {
        *(.bss .bss.*)
        *(.sbss .sbss.*) /* s for small bss */
    }
    PROVIDE(end = .); /* 定义BSS段的结束地址符号，也代表内核的结束地址 */

    /* 8. 定义启动栈 (Boot Stack) */
    /* 在内核数据末尾分配一个4KB的栈空间 */
    . = ALIGN(16);
    . += 4096; /* 栈大小为4KB */
    PROVIDE(stack_top = .); /* 定义栈顶地址符号，供entry.S使用 */
}