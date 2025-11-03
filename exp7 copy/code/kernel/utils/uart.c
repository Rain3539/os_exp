#include "../type.h"
#include "../def.h"
#define WriteReg(reg, v) (*(reg) = (v))


#define UART_BASE 0x10000000
#define UART_THR  (UART_BASE + 0)  
#define UART_LSR  (UART_BASE + 5)  

void uart_putc(char c) {
    volatile uint8 *thr = (volatile uint8*)UART_THR;   
    volatile uint8 *lsr = (volatile uint8*)UART_LSR;
    while ((*lsr & 0x20) == 0);
    
    WriteReg(thr, c);
}

