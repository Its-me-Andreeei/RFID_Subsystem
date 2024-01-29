#include <lpc22xx.h>
#include <stdio.h>
#include "utils/ringbuf.h"
#include "hal/timer.h"
#include "hal/uart0.h"
#include "utils/timer_software.h"

#define UART1_BUFFER_SIZE 1024

uint8_t uart1_buffer[UART1_BUFFER_SIZE];
tRingBufObject uart1_ringbuff_rx;


void UART1_irq(void) __irq
{	
	uint8_t ch;
	uint8_t iir;
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
	static volatile unsigned int i = 0;
	if ((T0IR & 1) != 0)
	{
		if (i % 2)
		{
			IO0SET = 1u << 30;	
		}
		else
		{
			IO0CLR = 1u << 30;
		}
		i++;
		TIMER_SOFTWARE_ModX();
		T0IR |= 1;
	}
}

void InitInterrupt()
{
	VICIntEnable |= (1 << 4) | (1 << 7);
	VICIntSelect |= 1 << 4;
	VICVectCntl0 = 7 | (1 << 5);
	VICVectAddr0 = (unsigned long int)UART1_irq;
	
}


int main(void)
{	
	volatile unsigned int i = 0;
	timer_software_handler_t timer1;
	TIMER_SOFTWARE_init();
	TIMER_Init();
	UART0_Init();
	InitInterrupt();
	
	PINSEL1 &= ~((1 << 29) | (1 << 28));
	IO0DIR |= 1u << 30;

	RingBufInit(&uart1_ringbuff_rx, uart1_buffer, UART1_BUFFER_SIZE);
	
	timer1 = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer1, MODE_1, 2000, 1);
	TIMER_SOFTWARE_reset_timer(timer1);
	TIMER_SOFTWARE_start_timer(timer1);
	
	while(1)
	{
		if (TIMER_SOFTWARE_interrupt_pending(timer1))
		{
			printf("test timer software\n");
			TIMER_SOFTWARE_clear_interrupt(timer1);
		}
	}
	
	
	while(1)
	{
		for (i = 0; i < 1000000; i++);
		printf ("test\n");
	}
	return 0;
}

