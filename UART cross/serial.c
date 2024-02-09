#include <lpc22xx.h>
#include "serial.h"

#define CR	10

void sendchar_UART0 (int ch)  
{              
	while (!(U0LSR & 0x20));
	U0THR = ch;
}

void sendchar_UART1 (int ch)  
{                 
	while (!(U1LSR & 0x20));
	U1THR = ch;
}


void init_uart01()
{
	PINSEL0 = 0x00050005;

	U0LCR = 0x83;
	U0DLL = 8;
	U0LCR = 0x03;
	U0FCR = 0x01;					// fifo enable



	U1LCR = 0x83;
	U1DLL = 8;
	U1LCR = 0x03;
	U1FCR = 0x01;					// fifo enable

}

void UART_cross(void)
{
	while(1)
	{
		if (U0LSR & 0x01)
		{
			//sendchar_UART0(U0RBR);
			sendchar_UART1(U0RBR);
			//U1THR = U0RBR;
		}
		if (U1LSR & 0x01)
		{
			sendchar_UART0(U1RBR);
			//U0THR = U1RBR;
		}
	}
}
