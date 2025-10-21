#include "console.h"
#include "../type.h"
#include <stdarg.h>
#include "../def.h"

#define BACKSPACE 0x100

void
cons_putc(int c)
{
  if(c == BACKSPACE){
    // if the user typed backspace, overwrite with a space.
    uart_putc('\b'); uart_putc(' '); uart_putc('\b');
  } else {
    uart_putc(c);
  }
}
void cons_puts(const char *s){
    while(*s){
        cons_putc(*s);
        s++;
    }
}


