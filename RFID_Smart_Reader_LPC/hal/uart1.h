#ifndef __UART0_H
#define __UART0_H

#include <stdint.h>

void UART1_Init(void);
int UART1_sendchar(int ch);
void UART1_sendbuffer(uint8_t *buffer, uint32_t size);
#endif

