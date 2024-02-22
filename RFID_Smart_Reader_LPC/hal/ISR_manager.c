#include "ISR_manager.h"
#include <lpc22xx.h>
#include "../utils/timer_software.h"
#include <stdio.h>

#define UART1_BUFFER_SIZE 1024

static uint8_t uart1_buffer[UART1_BUFFER_SIZE];
tRingBufObject uart1_ringbuff_rx; /*TBD: To be added into an interface later for better encapsulation*/

/*************************************************************************************************************************************************
	Function: 		UART1_irq
	Description:	This is the ISR handler in case a new frame is being received (RX ISR). Data is transfered from HW buffer to SW buffer for further processing
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void UART1_irq(void) __irq
{	
	uint8_t ch;
	uint8_t iir;
	
	/* dummy read to ack interrupt for UART*/
	
	iir = U1IIR; 
	/*RDA interrupt*/
	if (((iir >> 1) & 7) == 2)
	{		
		ch = U1RBR;	
		RingBufWriteOne(&uart1_ringbuff_rx, ch);
	}
	/* ack interrupt for VIC */
	VICVectAddr = 0; 
}

/*************************************************************************************************************************************************
	Function: 		TIMER_irq
	Description:	This function is an ISR handler called each 1 ms. This is used by software timer 
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void TIMER_irq(void) __irq
{
	if ((T0IR & 1) != 0)
	{
		TIMER_SOFTWARE_ModX();
		T0IR |= 1;
	}
}

/*************************************************************************************************************************************************
	Function: 		InitInterrupt
	Description:	This will initialise the Interrupt module (the VIC module)
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void InitInterrupt(void)
{
	RingBufInit(&uart1_ringbuff_rx, uart1_buffer, UART1_BUFFER_SIZE);
	
	VICIntEnable |= (1 << 4) | (1 << 7);
	VICIntSelect |= 1 << 4;
	VICVectCntl0 = 7 | (1 << 5);
	VICVectAddr0 = (unsigned long int)UART1_irq;
}
