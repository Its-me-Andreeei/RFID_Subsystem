#ifndef __UART0_H
#define __UART0_H

#include "../PlatformTypes.h"

void UART0_Init(void);
uint8_t UART0_sendchar(u8 ch);

#endif

