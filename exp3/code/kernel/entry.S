/* kernel/entry.S - RISC-V OS Entry Point */

.section .text.entry
.globl _start /* 声明_start为全局符号，供链接器使用 */

_start:
    # 1. 设置栈指针 (Set up stack pointer)
    #    链接脚本在内核末尾定义了stack_top符号。
    #    栈是向下增长的，所以我们将sp指向最高地址。
    la sp, stack_top

    # 2. 清零BSS段 (Clear the .bss section)
    #    BSS段存放C语言中未初始化的全局变量和静态变量，
    #    标准要求它们在程序开始时为零。
    la a0, sbss      # a0 = BSS起始地址
    la a1, end       # a1 = BSS结束地址
    
    # 循环清零
.bss_zero_loop:
    bgeu a0, a1, .bss_zero_done  # 如果 a0 >= a1, 跳转到完成
    sd zero, 0(a0)               # 将8字节的0写入a0指向的地址
    addi a0, a0, 8               # a0 += 8，移动到下一个8字节
    j .bss_zero_loop             # 继续循环

.bss_zero_done:

    # 3. 跳转到C语言主函数 (Jump to C main function)
    call kmain

# 4. 容错处理：如果kmain意外返回，则进入死循环
#    一个正常的内核永远不应该从它的主函数返回。
_hang:
    j _hang