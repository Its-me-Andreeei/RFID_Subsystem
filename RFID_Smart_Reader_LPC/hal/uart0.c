#include "uart0.h"
#include <lpc22xx.h>


/*************************************************************************************************************************************************
	Function: 		UART0_Init
	Description:	This function initialises UART0, which is used for DEBUGGING purposes (logging data)
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void UART0_Init()
{
  PINSEL0 |= 0x00000005;           
  U0LCR = (u8)0x83U;                   
  U0DLL = (u8)1U;    // BAUD = 921600 bps (max)
  U0LCR = (u8)0x03U;
  U0FCR = (u8)0x01U;	// fifo enable
}

/*************************************************************************************************************************************************
	Function: 		UART0_sendchar
	Description:	This function sends one byte over UART0 (for debugging/logging purposes)
	Parameters: 	characher to be send over UART0
	Return value:	characher to be send over UART0
							
*************************************************************************************************************************************************/
u8 UART0_sendchar(u8 ch)
{
	while (!((u8)U0LSR & (u8)0x20));
	return (U0THR = ch);
}

