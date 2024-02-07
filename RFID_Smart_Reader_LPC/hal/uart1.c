#include "uart1.h"
#include <lpc22xx.h>
#include <stdint.h>
#include "../utils/ringbuf.h"
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
	printf("TX: ");
	for(uint16_t i = 0; i < size; i++)
	{
		printf("%X ", buffer[i]);
	}
	printf("\n");
	//#endif /*END DEBUG_TX_RX*/
	//RingBufFlush(&uart1_ringbuff_rx); /*Clear buffer before starting a new transmission*/
	
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
	#define RXFE (uint8_t)7U /*Error in Rx FIFO*/
	#ifdef DEBUG
	#define FE (uint8_t)3U   /*Framing Error*/
	#define PE (uint8_t)2U   /*Parity Error*/
	#define OE (uint8_t)1U   /*Overrun Error*/
	#endif /*END DEBUG*/
	
	bool_t result = TRUE; 
	uint8_t error_status = (uint8_t)0U;
	/*This macros are local in order to reduce their scope. They are only used in this function for the moment*/
	TIMER_SOFTWARE_Wait(1000);
	
	(void)timeoutMs; /*TBD: To be implemented with timers after basic functionality is validated*/
	
	*actualLength = RingBufUsed(&uart1_ringbuff_rx);
	
	if(*actualLength == expectedLength)
	{
		RingBufRead(&uart1_ringbuff_rx, message, *actualLength);
	}
	error_status = (uint8_t)U1LSR;
	if((error_status & ((uint8_t)1 << RXFE)) != (uint8_t)0U)
	{
		#ifdef DEBUG
		if((error_status & ((uint8_t)1 << FE)) != (uint8_t)0U)
		{
			printf("Frame Error detected\n");
		}
		if((error_status & ((uint8_t)1 << PE)) != (uint8_t)0U)
		{
			printf("Parity Error detected\n");
		}
		if((error_status & ((uint8_t)1 << OE)) != (uint8_t)0U)
		{
			printf("Data Overrun Error detected\n");
		}
		#endif /*END DEBUG*/
		
		result = FALSE;
	}
	//#ifdef DEBUG_TX_RX
	printf("RX: ");
	printf ("\n%d = %d \n", *actualLength, expectedLength);
	for(uint16_t i = 0; i < *actualLength; i++)
	{
		printf("---%X ", message[i]);
	}
	printf("\n");
	//#endif /*END DEBUG_TX_RX*/
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
