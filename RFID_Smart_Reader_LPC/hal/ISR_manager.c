#include "ISR_manager.h"
#include <lpc22xx.h>
#include "../utils/timer_software.h"
#include <stdio.h>

#define UART1_BUFFER_SIZE 1024

static uint8_t uart1_buffer[UART1_BUFFER_SIZE];
tRingBufObject uart1_ringbuff_rx; /*TBD: To be added into an interface later for better encapsulation*/

void UART1_irq(void) __irq
{	
	uint8_t ch;
	uint8_t iir;
	printf("ISR\n");
	iir = U1IIR; // dummy read to ack interrupt for UART
	if (((iir >> 1) & 7) == 2) // RDA interrupt
	{		
		ch = U1RBR;	
		RingBufWriteOne(&uart1_ringbuff_rx, ch);
	}
	VICVectAddr = 0; // ack interrupt for VIC
}

void TIMER_irq(void) __irq
{
	if ((T0IR & 1) != 0)
	{
		TIMER_SOFTWARE_ModX();
		T0IR |= 1;
	}
}

void InitInterrupt(void)
{
	RingBufInit(&uart1_ringbuff_rx, uart1_buffer, UART1_BUFFER_SIZE);
	
	VICIntEnable |= (1 << 4) | (1 << 7);
	VICIntSelect |= 1 << 4;
	VICVectCntl0 = 7 | (1 << 5);
	VICVectAddr0 = (unsigned long int)UART1_irq;
}
