#ifndef __UART0_H
#define __UART0_H

#include <stdint.h>

void UART0_Init(void);
uint8_t UART0_sendchar(uint8_t ch);

#endif

