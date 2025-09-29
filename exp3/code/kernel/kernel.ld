/* kernel/kernel.ld - Advanced Linker Script for RISC-V OS */

OUTPUT_ARCH(riscv)
ENTRY(_start)

/* * [修改] 将基地址提高到 0x80200000，以避开 QEMU 默认加载的 OpenSBI 固件 (它在 0x80000000)。
 * OpenSBI 会在完成初始化后跳转到这个地址来执行内核。
 */
BASE_ADDRESS = 0x80000000;

/* * 定义程序头（Program Headers），告诉加载器如何映射段到内存
 * PT_LOAD: 表示这是一个需要加载到内存的段
 * FLAGS(5): R+X = 4+1 = 5 (读+执行)
 * FLAGS(6): R+W = 4+2 = 6 (读+写)
 */
PHDRS
{
    text_segment PT_LOAD FLAGS(5);  /* RX segment for code and rodata */
    data_segment PT_LOAD FLAGS(6);  /* RW segment for data and bss */
}

SECTIONS
{
    . = BASE_ADDRESS;

    /* 代码段和只读数据段，映射到 "text_segment" */
    .text : {
        *(.text.entry)
        *(.text .text.*)
    } :text_segment

    .rodata : {
        . = ALIGN(4096); /* 页对齐，好习惯 */
        PROVIDE(etext = .);
        *(.rodata .rodata.*)
    } :text_segment

    /* 可读写数据段，映射到 "data_segment" */
    .data : {
        . = ALIGN(4096); /* 页对齐 */
        *(.data .data.*)
    } :data_segment
  
    .bss : {
        PROVIDE(sbss = .);
        *(.sbss .sbss.*)
        *(.bss .bss.*)
    } :data_segment
    PROVIDE(end = .);

    /* 启动栈（不属于任何加载段）*/
    . = ALIGN(16);
    . += 4096;
    PROVIDE(stack_top = .);
}