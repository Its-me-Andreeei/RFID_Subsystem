#include "uart0.h"
#include <lpc22xx.h>


void UART0_Init()
{
  PINSEL0 |= 0x00000005;           
  U0LCR = 0x83;                   
  U0DLL = 1;                     // BAUD = 921600 bps (max)
  U0LCR = 0x03;
  U0FCR = 0x01;	// fifo enable
}

int UART0_sendchar(int ch)
{
	while (!(U0LSR & 0x20));
	return (U0THR = ch);
}


