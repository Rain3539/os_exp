# 最小RISC-V操作系统启动汇编代码

.section .text
.global _start

_start:
    # 调试检查点1：输出启动标记 'S'
    li t0, 0x10000000      # UART基地址
    li t1, 'S'             # 启动标记
    sb t1, 0(t0)           # 输出字符S表示启动
    li t1, '\n'            # 换行
    sb t1, 0(t0)           

    # 首先清零BSS段 - 必须在使用栈之前完成
    # 因为stack0在BSS段中，如果先使用栈再清零BSS会破坏栈数据
    la t0, __bss_start     # BSS段起始地址
    la t1, __bss_end       # BSS段结束地址
    
clear_bss_loop:
    beq t0, t1, clear_bss_done  # 如果清零完成，跳出循环
    sb zero, 0(t0)              # 将当前地址清零
    addi t0, t0, 1              # 地址递增
    j clear_bss_loop            # 继续循环

clear_bss_done:
    # 调试检查点2：BSS清零完成标记
    li t0, 0x10000000      # 重新加载UART地址
    li t1, 'B'             # BSS清零完成标记
    sb t1, 0(t0)           # 输出字符B
    li t1, '\n'            # 换行
    sb t1, 0(t0)           

    # 现在可以安全地设置栈指针了
    # BSS已经清零，stack0区域是干净的
    la sp, stack0
    li a0, 1024*4
    add sp, sp, a0

    # 调试检查点3：验证栈设置完成
    li t1, 'S'             # 栈设置完成标记  
    sb t1, 0(t0)           # 输出字符S表示栈设置完成
    li t1, '\n'            # 换行
    sb t1, 0(t0) 

    # 现在可以安全地跳转到C主函数了
    call start              # 调用main()函数




halt:
    # 输出错误标记
    li t0, 0x10000000      
    li t1, 'E'             # 错误标记：main不应该返回
    sb t1, 0(t0)           
    j halt                 # 无限循环

