#include "uart1.h"
#include <lpc22xx.h>
#include <stdint.h>
#include "../utils/ringbuf.h"
#include "ISR_manager.h"
#include <stdio.h>
#include "../utils/timer_software.h"

static timer_software_handler_t timer_RX;
static uint8_t commErrors = 0;

void UART1_Init(void)
{
	PINSEL0 |= 0x50000;   //Enable TX, RX        
	U1LCR = 0x83;                   
	U1DLL = 8;                     // BAUD = 115200 bps (max)
	U1LCR = 0x03;
	U1FCR = 0x01;	// fifo enable
	U1IER = 1;
	
	timer_RX = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer_RX, MODE_0, 2000, 1);
}

uint8_t UART1_get_CommErrors(void)
{
	return commErrors;
}

uint8_t UART1_sendchar(uint8_t ch)
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
bool_t UART1_sendbuffer(const uint8_t *buffer, const uint32_t size, const uint32_t timeoutMs)
{
	(void)timeoutMs; /*TBD: To be implemented with timers after basic functionality is validated*/
	printf("------------\n");
	printf("TX: ");
	for(uint16_t i = 0; i < size; i++)
	{
		printf("%02X ", buffer[i]);
	}
	printf("\n");
	
	for (uint32_t i = 0; i < size; i++)
	{
		(void)UART1_sendchar(buffer[i]);	
	}
	
	return TRUE;
}

/**************************************************************************
*		Function: UART1_receivebuffer
*		Parameters: 
*							message: The buffer where received results will be stored dor further processing [OUT]
*							expectedLength: The expected length of the RX buffer [IN]
*							timeoutMS: The time in MS in which the buffer must be completly sent [IN]
*							actualLength: The actual number of processed bytes of the RX buffer. Will be lesser or equal to expectedLength param [OUT]
* 						timeoutMS: The time in MS in which the buffer must be completly sent [IN]
*
*   Return value:
*							TRUE = Success in trasmission
*							FALSE = The buffer was not transmitted wihin TimeoutMS miliseconds OR COMM errors happened
**************************************************************************/

bool_t UART1_receivebuffer(uint8_t* message, uint32_t expectedLength, uint32_t* actualLength, const uint32_t timeoutMs)
{
	/*This macros are local in order to reduce their scope. They are only used in this function for the moment*/
	#define RXFE ((uint8_t)7U) /*Error in Rx FIFO*/
	
	bool_t result = TRUE; 
	uint8_t error_status = (uint8_t)0U;
	uint16_t ringBuffLength = (uint8_t)0U;
	uint8_t index = (uint8_t)0U;
	
	
	(void)timeoutMs; /*TBD: To be implemented with timers after basic functionality is validated*/
	
	TIMER_SOFTWARE_reset_timer(timer_RX);
	TIMER_SOFTWARE_start_timer(timer_RX);
	
	while(TIMER_SOFTWARE_interrupt_pending(timer_RX) == (uint8_t)0x00U)
	{
		ringBuffLength = RingBufUsed(&uart1_ringbuff_rx);
		
		if(ringBuffLength >= expectedLength)
		{
			RingBufRead(&uart1_ringbuff_rx, message, expectedLength);
			index = expectedLength;
			break;
		}
		else{ //read one byte a time
			if((index < expectedLength) && (RingBufUsed(&uart1_ringbuff_rx) > 0))
			{
				message[index] = RingBufReadOne(&uart1_ringbuff_rx);
				index++;
			}
			else if(index == expectedLength)
			{
				break; //we got all [expectedLength] bytes
			}
		}
	}
	
	if(index < expectedLength)
	{
		result = FALSE;
		if(actualLength != NULL)
		{
			*actualLength = index;
		}
	}
	else
	{
		if(actualLength != NULL)
		{
			*actualLength = expectedLength;
		}
	}
	
	error_status = (uint8_t)U1LSR;
	if((error_status & ((uint8_t)1 << RXFE)) != (uint8_t)0U)
	{
		result = FALSE;
	}
	
	if(FALSE == result)
	{
		commErrors++;
	}
	
	if(actualLength != NULL)
	{
		printf("RX: ");
		for(uint16_t i = 0; i < *actualLength; i++)
		{
			printf("%02X ", message[i]);
		}
		printf("\n");
	}

	return result;
}

bool_t UART1_flush(void)
{
	
	//#define RxFIFO_RST ((uint8_t)1U)
	//#define TxFIFO_RST ((uint8_t)2U)
	//U1FCR |= ((uint8_t)(((uint8_t)1U <<RxFIFO_RST) | ((uint8_t)1U <<TxFIFO_RST))); /*Perform TX and RX reset on FIFOs on HW level*/
	/*TBD: Check if flush is performed by checking EMPTY flag within a timeout. If no flush is done -> HW issue*/
	/*dummy return*/
	RingBufFlush(&uart1_ringbuff_rx);
	return TRUE;
}

uint16_t UART1_get_RX_buffer_size(void)
{
	/*16 bits is enough to represent maximum size, which is configured to be 1024 bytes in this case*/
	return (uint16_t)RingBufUsed(&uart1_ringbuff_rx);
}
