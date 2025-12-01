# RISC-V OS引导入口点
# 此文件包含系统引导时运行的初始汇编代码
# 在跳转到C代码之前设置基本运行时环境

.section .text           # 将代码放在文本段
.global _entry          # 使_entry符号对链接器全局可见
_entry:                 # 引导入口点 - CPU执行的第一段代码
    
    # 步骤1: 初始化栈指针
    # 栈指针(sp)必须在任何函数调用之前设置
    # _stack_top在kernel.ld链接器脚本中定义
    la sp, _stack_top   # 将栈顶地址加载到栈指针
    
    # 步骤2: 清除BSS(Block Started by Symbol)段
    # BSS包含未初始化的全局变量，应该为零
    # 这确保所有全局变量以可预测的值开始
    la a0, _sbss        # 将BSS段起始地址加载到a0
    la a1, _ebss        # 将BSS段结束地址加载到a1
    bgeu a0, a1, zero_bss_done  # 如果BSS为空(起始>=结束)则跳过
    
zero_bss_loop:
    sw zero, (a0)       # 在a0地址处存储零字
    addi a0, a0, 4      # 移动到下一个字(4字节)
    bltu a0, a1, zero_bss_loop  # 如果a0 < a1则继续
zero_bss_done:

    # 步骤3: 跳转到C主函数
    # 所有汇编初始化完成，将控制权转移到C代码
    call start          # 调用start.c中的start()函数

    # 步骤4: 无限循环(不应该到达这里)
    # 如果start()函数返回，进入无限循环以防止未定义行为
spin:
    j spin              # 跳转到自身 - 无限循环