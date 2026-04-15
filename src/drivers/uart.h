#include <stdlib.h>
#include <stdint.h>

#define UART_DATA_REG ((volatile uint32_t *)0x09000000)
#define UART_FLAG_REG ((volatile uint32_t *)0x09000018)

//param string debug logs string
//info this function will add the debug logs string char by char in UART buffer
void uart_puts(char * string);

void uart_putc(char c);