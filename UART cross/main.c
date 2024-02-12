#include <lpc22xx.h>
#include "serial.h"

int main(void)
{
	char test;
	PINSEL0 = 0x00000000;
	PINSEL1 = 0x00000000;
	init_uart01();
	for(;;)
	{
		break;
		sendchar_UART1(test++);
	}
	UART_cross();
}