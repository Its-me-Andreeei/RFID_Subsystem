#include "uart1.h"
#include <lpc22xx.h>
#include <stdint.h>
#include "../utils/ringbuf.h"
#include "../utils/timer_software.h"
#include "ISR_manager.h"
#include <stdio.h>

void UART1_Init(void)
{
	PINSEL0 |= 0x50000;   //Enable TX, RX        
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

/**************************************************************************
*		Function: UART1_sendbuffer
*		Parameters: 
*							buffer: The buffer to be sent [IN]
*							size: The length of the buffer [IN]
*							timeoutMS: The time in MS in which the buffer must be completly sent [IN]
*
* 
*   Return value:
*							TRUE = Success in trasmission
**************************************************************************/
bool_t UART1_sendbuffer(uint8_t *buffer, uint32_t size, const uint32_t timeoutMs)
{
	(void)timeoutMs; /*TBD: To be implemented with timers after basic functionality is validated*/
	//#ifdef DEBUG_TX_RX
	printf("---------------\n");
	printf("TX: ");
	for(uint16_t i = 0; i < size; i++)
	{
		printf("%X ", buffer[i]);
	}
	printf("\n");
	//#endif /*END DEBUG_TX_RX*/
	
	for (uint32_t i = 0; i < size; i++)
	{
		UART1_sendchar(buffer[i]);	
	}
	
	return TRUE;
}

/**************************************************************************
*		Function: UART1_receivebuffer
*		Parameters: 
*							message: The buffer where received results will be stored dor further processing [OUT]
*							expectedLength: The expected length of the RX buffer [IN]
*							timeoutMS: The time in MS in which the buffer must be completly sent [IN]
*							actualLength: The actual length of the RX buffer [OUT]
* 						timeoutMS: The time in MS in which the buffer must be completly sent [IN]
*
*   Return value:
*							TRUE = Success in trasmission
*							FALSE = The buffer was not transmitted wihin TimeoutMS miliseconds OR COMM errors happened
**************************************************************************/
bool_t UART1_receivebuffer(uint8_t* message, uint32_t expectedLength, uint32_t* actualLength, const uint32_t timeoutMs)
{
	bool_t result = TRUE; 
	uint8_t error_status = (uint8_t)0U;
	timer_software_handler_t timer_RX;
	
	#define RXFE (uint8_t)7U /*Error in Rx FIFO*/
	
	(void)timeoutMs; /*TBD: To be implemented with timers after basic functionality is validated*/
	
	timer_RX = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer_RX, MODE_0, 2000, 1); /*TBD: To be added timeoutMS variable after testing is done. Might require adjusting in order to give enough time to UART Comm to happen*/
	TIMER_SOFTWARE_reset_timer(timer_RX);
	
	while(!TIMER_SOFTWARE_interrupt_pending(timer_RX)) /*While interrupt is not happening -> Timeout not reached yet*/
	{
		/*Fetch current length from ISR buffer*/
		*actualLength = RingBufUsed(&uart1_ringbuff_rx);
		
		/*Since Mercury API splits the expected length by first computing if 5 bytes (minimum required) are received, then checks for data bytes also*/
		/*We check that we received AT LEAST expectedLength (but can be more out there) */
		if(*actualLength >= expectedLength)
		{
			RingBufRead(&uart1_ringbuff_rx, message, expectedLength);
			/*After fatching is complete we disable and clear the timer. It would remain PENDING if code does not reach here*/
			/*Meaning time is out and COMM is not finished yet*/
			TIMER_SOFTWARE_clear_interrupt(timer_RX);
			TIMER_SOFTWARE_disable_timer(timer_RX);
			break;
		}
	}
	
	/*check if RX did not finished within timer's timeout, there is an error*/
	if(TIMER_SOFTWARE_interrupt_pending(timer_RX))  
	{
		result = FALSE; /*Timeout error happened*/
		TIMER_SOFTWARE_clear_interrupt(timer_RX);
		TIMER_SOFTWARE_disable_timer(timer_RX);
	}
	TIMER_SOFTWARE_clear_interrupt(timer_RX);
	TIMER_SOFTWARE_disable_timer(timer_RX);
	
	error_status = (uint8_t)U1LSR;
	if((error_status & ((uint8_t)1 << RXFE)) != (uint8_t)0U)
	{
		printf("\nComm Error in RX\n");
		result = FALSE; /*Comm error happened*/
	}
	//#ifdef DEBUG_TX_RX
	printf("RX: ");
	for(uint16_t i = 0; i < expectedLength; i++)
	{
		printf("%02X ", message[i]);
	}
	printf("\n");
	//#endif /*END DEBUG_TX_RX*/
	
	TIMER_SOFTWARE_release_timer(timer_RX);
	return result;
}

bool_t UART1_flush(void)
{
	/*We only flush the software buffer, not the HW one*/
	RingBufFlush(&uart1_ringbuff_rx);
	return ((RingBufEmpty(&uart1_ringbuff_rx) == TRUE) ? TRUE : FALSE);
}
