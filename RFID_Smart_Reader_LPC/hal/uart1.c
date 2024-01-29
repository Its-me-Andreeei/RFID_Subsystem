#include "uart1.h"
#include <lpc22xx.h>
#include <stdint.h>

void UART1_Init()
{
	PINSEL0 |= 0x00000005;           
	U1LCR = 0x83;                   
	U1DLL = 8;                     // BAUD = 115200 bps (max)
	U1LCR = 0x03;
	U1FCR = 0x01;	// fifo enable
	U1IER = 1;
}

int UART1_sendchar(int ch)
{
	while (!(U1LSR & 0x20));
	return (U1THR = ch);
}

void UART1_sendbuffer(uint8_t *buffer, uint32_t size)
{
	for (uint32_t i = 0; i < size; i++)
	{
		UART1_sendchar(buffer[i]);	
	}
}


