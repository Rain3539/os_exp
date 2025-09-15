.section .text.start
.global _start

_start:
    # 设置栈指针
    li sp, 0x80010000
    
    # 直接调用main函数
    call kmain
    
    # 死循环
halt:
    j halt
