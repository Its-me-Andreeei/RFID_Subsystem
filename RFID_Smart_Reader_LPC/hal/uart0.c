#include "uart0.h"
#include <lpc22xx.h>

void UART0_Init()
{
  PINSEL0 |= 0x00000005;           
  U0LCR = (uint8_t)0x83U;                   
  U0DLL = (uint8_t)1U;    // BAUD = 921600 bps (max)
  U0LCR = (uint8_t)0x03U;
  U0FCR = (uint8_t)0x01U;	// fifo enable
}

uint8_t UART0_sendchar(uint8_t ch)
{
	while (!((uint8_t)U0LSR & (uint8_t)0x20));
	return (U0THR = ch);
}

