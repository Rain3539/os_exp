#
# kernelvec.S: 内核态陷阱的汇编入口点
# 当在 S 模式下发生中断或异常时，CPU会跳转到这里。
#
.globl kerneltrap
.globl kernelvec
.align 4
kernelvec:
        # 步骤1: 保存上下文 (Context Save)
        # 在调用C函数之前，必须保存所有可能被C函数破坏的寄存器（调用者保存寄存器）。
        # 同时，也需要保存一些状态寄存器，以便后续恢复现场。

        # 在内核栈上分配256字节的空间用于保存寄存器。
        addi sp, sp, -256

        # 保存通用寄存器到栈上。
        sd ra, 0(sp)  # 返回地址
        # sd sp, 8(sp)不需要保存，因为它在函数调用过程中会改变，但在返回时会恢复。
        sd gp, 16(sp)
        sd tp, 24(sp)
        sd t0, 32(sp)
        sd t1, 40(sp)
        sd t2, 48(sp)
        sd a0, 72(sp)
        sd a1, 80(sp)
        sd a2, 88(sp)
        sd a3, 96(sp)
        sd a4, 104(sp)
        sd a5, 112(sp)
        sd a6, 120(sp)
        sd a7, 128(sp)
        sd t3, 216(sp)
        sd t4, 224(sp)
        sd t5, 232(sp)
        sd t6, 240(sp)


        # 步骤2: 调用C语言的陷阱处理函数
        # 将当前栈指针 (sp) 作为参数传递给 kerneltrap。
        # C代码可以通过这个指针访问到所有保存的寄存器，形成一个 trapframe 结构。
        mv a0, sp
        call kerneltrap



        # 步骤3: 恢复上下文 (Context Restore)
        # C函数返回后，从栈上恢复所有之前保存的寄存器。
        # 恢复的顺序与保存时相反。
        ld ra, 0(sp)
        # ld sp, 8(sp)
        ld gp, 16(sp)
        # not tp (contains hartid), in case we moved CPUs
        ld t0, 32(sp)
        ld t1, 40(sp)
        ld t2, 48(sp)
        ld a0, 72(sp)  
        ld a1, 80(sp)
        ld a2, 88(sp)
        ld a3, 96(sp)
        ld a4, 104(sp)
        ld a5, 112(sp)
        ld a6, 120(sp)
        ld a7, 128(sp)
        ld t3, 216(sp)
        ld t4, 224(sp)
        ld t5, 232(sp)
        ld t6, 240(sp)
        # 释放栈空间
        addi sp, sp, 256

        # 步骤4: 从陷阱返回
        # sret (Supervisor-mode return from trap) 指令会执行以下原子操作：
        # 1. 将 PC (程序计数器) 的值设置为 sepc 寄存器的值。
        # 2. 将权限级别恢复到 sstatus.SPP 中保存的模式 (S或U)。
        # 3. 重新启用中断 (如果 sstatus.SPIE 为1)。
        sret