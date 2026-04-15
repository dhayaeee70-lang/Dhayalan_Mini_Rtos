#include "uart.h"

void uart_putc(char c){
    while((*UART_FLAG_REG) & (1<<5));

    *UART_DATA_REG = c;
}

void uart_puts(char * str){
    while(*str){
        uart_putc(*str++);
    }
}
