//
// 16550a UART的低级驱动例程
//

#include "kernel.h"
#include "memlayout.h"
#include "riscv.h"

// UART控制寄存器是内存映射的
// 在地址UART0处。此宏返回
// 其中一个寄存器的地址
#define Reg(reg) ((volatile unsigned char *)(UART0 + (reg)))

// UART控制寄存器
// 有些在读取和写入时有不同的含义
// 参见 http://byterunner.com/16550.html
#define RHR 0                 // 接收保持寄存器(用于输入字节)
#define THR 0                 // 发送保持寄存器(用于输出字节)
#define IER 1                 // 中断使能寄存器
#define IER_RX_ENABLE (1<<0)  // 接收中断使能
#define IER_TX_ENABLE (1<<1)  // 发送中断使能
#define FCR 2                 // FIFO控制寄存器
#define FCR_FIFO_ENABLE (1<<0)  // FIFO使能
#define FCR_FIFO_CLEAR (3<<1) // 清除两个FIFO的内容
#define ISR 2                 // 中断状态寄存器
#define LCR 3                 // 线路控制寄存器
#define LCR_EIGHT_BITS (3<<0) // 8位数据位
#define LCR_BAUD_LATCH (1<<7) // 设置波特率的特殊模式
#define LSR 5                 // 线路状态寄存器
#define LSR_RX_READY (1<<0)   // 输入等待从RHR读取
#define LSR_TX_IDLE (1<<5)    // THR可以接受另一个要发送的字符

#define ReadReg(reg) (*(Reg(reg)))    // 读寄存器宏
#define WriteReg(reg, v) (*(Reg(reg)) = (v))  // 写寄存器宏

// 用于传输
static struct spinlock tx_lock;  // 发送锁
static int tx_busy;              // UART是否正忙发送
static int tx_chan;              // &tx_chan是"等待通道"

extern volatile int panicking; // 来自printf.c
extern volatile int panicked;  // 来自printf.c

// UART初始化
void
uartinit(void)
{
  // 禁用中断
  WriteReg(IER, 0x00);

  // 设置波特率的特殊模式
  WriteReg(LCR, LCR_BAUD_LATCH);

  // 38.4K波特率的LSB
  WriteReg(0, 0x03);

  // 38.4K波特率的MSB
  WriteReg(1, 0x00);

  // 离开设置波特率模式，
  // 并设置字长为8位，无奇偶校验
  WriteReg(LCR, LCR_EIGHT_BITS);

  // 重置并使能FIFO
  WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

  // 使能发送和接收中断
  WriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE);

  initlock(&tx_lock, "uart");  // 初始化发送锁
}

// 将buf[]传输到uart。使用轮询方式等待UART准备好
// 简化实现，不依赖中断机制
void
uartwrite(char buf[], int n)
{
  acquire(&tx_lock);  // 获取发送锁

  int i = 0;
  while(i < n){ 
    // 轮询等待UART发送寄存器空闲（类似uartputc_sync）
    while((ReadReg(LSR) & LSR_TX_IDLE) == 0) {
      // 等待UART准备好
    }
      
    WriteReg(THR, buf[i]);  // 发送字符
    i += 1;
  }

  release(&tx_lock);  // 释放发送锁
}

// 不使用中断向uart写入一个字节，
// 供内核printf()和回显字符使用
// 它旋转等待uart的输出寄存器为空
void
uartputc_sync(int c)
{
  if(panicking == 0)
    push_off();  // 禁用中断

  if(panicked){
    for(;;)  // 恐慌状态下无限循环
      ;
  }

  // 等待LSR中的发送保持空位被设置
  while((ReadReg(LSR) & LSR_TX_IDLE) == 0)
    ;
  WriteReg(THR, c);  // 发送字符

  if(panicking == 0)
    pop_off();  // 恢复中断
}

// 从UART读取一个输入字符
// 如果没有等待，返回-1
int
uartgetc(void)
{
  if(ReadReg(LSR) & LSR_RX_READY){
    // 输入数据已就绪
    return ReadReg(RHR);  // 返回接收的字符
  } else {
    return -1;  // 无数据
  }
}

// 处理uart中断，因为输入已到达，
// 或uart准备好更多输出，或两者都有
// 从devintr()调用
void
uartintr(void)
{
  ReadReg(ISR); // 确认中断

  acquire(&tx_lock);  // 获取发送锁
  if(ReadReg(LSR) & LSR_TX_IDLE){
    // UART完成传输；唤醒发送线程
    tx_busy = 0;
    wakeup(&tx_chan);
  }
  release(&tx_lock);  // 释放发送锁

  // 读取和处理传入字符
  while(1){
    int c = uartgetc();  // 获取字符
    if(c == -1)
      break;
    consoleintr(c);  // 传递给控制台中断处理
  }
}

// 本实验的简化实现
void uart_init(void) {
    uartinit();  // 调用实际初始化函数
}

void uart_putc(char c) {
    uartputc_sync(c);  // 同步发送字符
}

void uart_puts(char *s) {
    while (*s) {
        uart_putc(*s++);  // 逐个字符发送字符串
    }
}

// 注意：其他函数在各自的文件中实现