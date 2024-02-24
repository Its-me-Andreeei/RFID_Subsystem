#include "uart1.h"
#include <lpc22xx.h>
#include <stdint.h>
#include "../utils/ringbuf.h"
#include "ISR_manager.h"
#include <stdio.h>
#include "../utils/timer_software.h"

/*This is used as a software timer, for RX operation*/
static timer_software_handler_t timer_TX_RX;

/*This counter is incremented whenever a communication error occours*/
static uint8_t commErrors = 0;

/*************************************************************************************************************************************************
	Function: 		UART1_Init
	Description:	This function is initialising UART1 module. Must be called before any other UART1 function
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void UART1_Init(void)
{
	PINSEL0 |= 0x50000;   //Enable TX, RX        
	U1LCR = 0x83;                   
	U1DLL = 8;                     // BAUD = 115200 bps (max)
	U1LCR = 0x03;
	U1FCR = 0x01;	// fifo enable
	U1IER = 1;
	
	/*Configure software timer used in receive operation. MODE_0 means counter will stop after reaching desired value. */
	/*2s is the default timeout but can be changed*/
	timer_TX_RX = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer_TX_RX, MODE_0, 2000, 1);
}

/*************************************************************************************************************************************************
	Function: 		UART1_get_CommErrors
	Description:	This function is returning the number of TX/RX errors (present or historic). Must not be called before UART1_init() function
	Parameters: 	void
	Return value: The number of Communication errors

*************************************************************************************************************************************************/
uint8_t UART1_get_CommErrors(void)
{
	return commErrors;
}

/*************************************************************************************************************************************************
	Function: 		UART1_sendchar
	Description:	This function is sending one byte only over UART1. Must not be called before UART1_init() function
	Parameters: 	character to be send
	Return value: The sent byte

*************************************************************************************************************************************************/
uint8_t UART1_sendchar(uint8_t ch)
{
	while (!(U1LSR & 0x20));
	return (U1THR = ch);
}

static UART_TX_RX_Status_en check_communication_errors(void)
{
	/*This macros are local in order to reduce their scope. They are only used in this function for the moment*/
	#define RXFE ((uint8_t)7U) /*Error in Rx FIFO*/
	
	uint8_t error_status = (uint8_t)0U;
	UART_TX_RX_Status_en result;
	
	error_status = (uint8_t)U1LSR;
	if((error_status & ((uint8_t)1 << RXFE)) != (uint8_t)0U)
	{
		commErrors++;
		result = COMMUNICATION_ERROR;
	}
	else
	{
		result = RETURN_OK;
	}

	return result;
}

/******************************************************************************************************************************************************************
		Function: UART1_sendbuffer
		Description: This function is sending a message using UART protocol, within a given timeout, with a known length. Must not be called before UART1_init() function
		Parameters: 
							buffer: The buffer to be sent [IN]
							size: The length of the buffer [IN]
							timeoutMS: The time in MS in which the buffer must be completly sent [IN]
 
		Return value:
							RETURN_OK = Success in trasmission: all bytes sent within timeout, no communication errors
							TIMEOUT_ERROR = Transmission was not send within given timeout. Check if BAUD or timeout is configured OK
*******************************************************************************************************************************************************************/
UART_TX_RX_Status_en UART1_sendbuffer(const uint8_t *buffer, const uint32_t size, const uint32_t timeoutMs)
{
	uint32_t txBufferIndex = 0x000000;
	UART_TX_RX_Status_en result = RETURN_OK;
	uint16_t i_dbg = 0;
	
	printf("------------\n");
	printf("TX: ");
	for(i_dbg = 0x0000; i_dbg < size; i_dbg++)
	{
		printf("%02X ", buffer[i_dbg]);
	}
	printf("\n");
	
	TIMER_SOFTWARE_reset_timer(timer_TX_RX);
	TIMER_SOFTWARE_configure_timer(timer_TX_RX, MODE_0, timeoutMs, 1);
	TIMER_SOFTWARE_start_timer(timer_TX_RX);
	
	/*As long as we have not send all bytes, and we still have time, send byte by byte through UART*/
	for (txBufferIndex = 0; (txBufferIndex < size) && (TIMER_SOFTWARE_interrupt_pending(timer_TX_RX) == 0x00); txBufferIndex++)
	{
		(void)UART1_sendchar(buffer[txBufferIndex]);	
	}
	
	/*If timer is over and we still have bytes to send, we failed to send all bytes within given timeout*/
	if((0U != TIMER_SOFTWARE_interrupt_pending(timer_TX_RX)) && (txBufferIndex < size))
	{
		result = TIMEOUT_ERROR;
	}
	else if( txBufferIndex == size) /*Even if timeout is over, it is acceptable to finish sending at time limit*/
	{
		result = RETURN_OK;
	}
	
	/*Clean interrupt flag as this might be used in other functions*/
	if(0U != TIMER_SOFTWARE_interrupt_pending(timer_TX_RX))
	{
		TIMER_SOFTWARE_clear_interrupt(timer_TX_RX);
	}
	
	/*Make sure timer leave this functions with TCNT = 0*/
	if(0U != TIMER_SOFTWARE_is_Running(timer_TX_RX))
	{
		TIMER_SOFTWARE_reset_timer(timer_TX_RX);
	}
	return result;
}

/*************************************************************************************************************************************************
	Function: 		UART1_send_reveice_PING
	Description:	This function is sending a PING message from MCU and is waiting for a specific response. The PING signal is not making any 
								configuration changes in destination system, and it's only used to prove if UART communication it's working
								Must not be called before UART1_init() function
	Parameters: 	void
	Return value:
								RETURN_OK = Response to PING signal was OK, with respect to given timeout
								RETURN_NOK = Response to PING was absent or not the expected one (might be corrupted)
								TIMEOUT_ERROR = incomplete response in given timeout
								COMMUNICATION_ERROR = There was a HW communication issue
*************************************************************************************************************************************************/
UART_TX_RX_Status_en UART1_send_reveice_PING(void)
{
	#define PING_SIZE ((uint8_t)5U)
	#define PING_RESPONSE_SIZE ((uint8_t)27U)
	
	const uint8_t PING[PING_SIZE] = {(uint8_t)0xFFU, (uint8_t)0x00U, (uint8_t)0x03U, (uint8_t)0x1DU, (uint8_t)0x0CU};
	uint8_t actual_response[PING_RESPONSE_SIZE];
	uint16_t actual_response_index = (uint16_t)0x0000U;
	UART_TX_RX_Status_en result = RETURN_OK;
	UART_TX_RX_Status_en comm_error_result = RETURN_OK;
	uint16_t loop_index = 0x0000;
	
	/*Make sure receiver buffer is empty*/
	UART1_flush();
	/*Send ping signal trough UART*/
	UART1_sendbuffer(PING, PING_SIZE, 1000);
	
	/*Use a fixed timeout to not get stuck in loop*/
	TIMER_SOFTWARE_reset_timer(timer_TX_RX);
	TIMER_SOFTWARE_start_timer(timer_TX_RX);
	
	while(!TIMER_SOFTWARE_interrupt_pending(timer_TX_RX))
	{
		/*Check for available data in ISR buffer*/
		if(!RingBufEmpty(&uart1_ringbuff_rx))
		{
			/*Guard in order to avoid overflow*/
			if(actual_response_index < PING_RESPONSE_SIZE)
			{
				/*Get availabe data from ISR buffer*/
				actual_response[actual_response_index] = RingBufReadOne(&uart1_ringbuff_rx);
				actual_response_index ++;
			}
			else
			{
				/*Reaching here means we got all expected bytes, we can leave earlier then TIMEOUT ms*/
				break;
			}
		}
	}
	
	/*Reset and clear interrupt, as this timer is also used by other functions*/
	TIMER_SOFTWARE_reset_timer(timer_TX_RX);
	TIMER_SOFTWARE_clear_interrupt(timer_TX_RX);
	
	/*Make sure we have exactly PING_RESPONSE_SIZE bytes*/
	if(actual_response_index == PING_RESPONSE_SIZE)
	{
		/*Check for meaningful bytes only (including CRC for optimized speed*/
		if((actual_response[0] == (uint8_t)0xFF) && 
			(actual_response[1] == (uint8_t)0x14) && 
			(actual_response[2] == (uint8_t)0x03) && 
			(actual_response[PING_RESPONSE_SIZE-2] == (uint8_t)0x79)  && 
			(actual_response[PING_RESPONSE_SIZE-1] == (uint8_t)0x62) )
		{
			result = RETURN_OK;
		}
		else
		{
			/*Something is not OK -> maybe corrupted message*/
			result = RETURN_NOK;
		}
			
		for(loop_index = 0x0000; loop_index < PING_RESPONSE_SIZE; loop_index++)
		{
			printf("%02X ", actual_response[loop_index]);
		}
	}
	else
	{
		/*If we got 0 or less bytes then expected within timoeut*/
		result = TIMEOUT_ERROR;
	}
	comm_error_result = check_communication_errors();
	
	/*Only change return of function if there is a communication HW issue*/
	if(comm_error_result != RETURN_OK)
	{
		result = comm_error_result;
	}
	return result;
}

/*************************************************************************************************************************************************
		Function: UART1_receivebuffer
		Description:
							This function waits for 'expectedLength' bytes to be available in RX buffer within a given timeout. If the timeout is over, 
							actualLength will be the length achieved until timeout expired. Must not be called before UART1_init() function
		Parameters: 
							message: The buffer where received results will be stored dor further processing [OUT]
							expectedLength: The expected length of the RX buffer [IN]
							timeoutMS: The time in MS in which the buffer must be completly sent [IN]
							actualLength: The actual number of processed bytes of the RX buffer. Will be lesser or equal to expectedLength param [OUT]
							timeoutMS: The time in MS in which the buffer must be completly sent [IN]

		Return value:
							RETURN_OK = Success in trasmission - all bytes received within given timeout
							TIMEOUT_ERROR = The buffer was not transmitted wihin TimeoutMS
							COMMUNICATION_ERROR = There was a HW communication issue
*************************************************************************************************************************************************/
UART_TX_RX_Status_en UART1_receivebuffer(uint8_t* message, uint32_t expectedLength, uint32_t* actualLength, const uint32_t timeoutMs)
{
	UART_TX_RX_Status_en result = RETURN_OK; 
	UART_TX_RX_Status_en comm_error_result = RETURN_OK;
	uint16_t ringBuffLength = (uint8_t)0U;
	uint8_t index = (uint8_t)0U;
	uint16_t index_dbg = 0x0000;
	
	TIMER_SOFTWARE_reset_timer(timer_TX_RX);
	
	/*We only want to set timeout with the parameter timeout, but in order to do it we have to set entire configuration for timer*/
	TIMER_SOFTWARE_configure_timer(timer_TX_RX, MODE_0, timeoutMs, 1);
	TIMER_SOFTWARE_start_timer(timer_TX_RX);
	
	while(TIMER_SOFTWARE_interrupt_pending(timer_TX_RX) == (uint8_t)0x00U)
	{
		ringBuffLength = RingBufUsed(&uart1_ringbuff_rx);
		
		if(ringBuffLength >= expectedLength)
		{
			RingBufRead(&uart1_ringbuff_rx, message, expectedLength);
			index = expectedLength;
			break;
		}
		else{ /*read one byte a time*/
			if((index < expectedLength) && (RingBufUsed(&uart1_ringbuff_rx) > 0))
			{
				message[index] = RingBufReadOne(&uart1_ringbuff_rx);
				index++;
			}
			else if(index == expectedLength)
			{
				/*If we got the bytes in time, stop timer and reset it (otherwise it will reset itself when timeout reached)*/
				TIMER_SOFTWARE_stop_timer(timer_TX_RX);
				TIMER_SOFTWARE_reset_timer(timer_TX_RX);
				
				break; /*we got all [expectedLength] bytes*/
			}
		}
	}

	if(index < expectedLength)
	{
		result = TIMEOUT_ERROR;
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
	
	/*Check for HW issues regarding communication errors*/
	comm_error_result = check_communication_errors();
	
	/*Only change return of function if there is a communication HW issue*/
	if(comm_error_result != RETURN_OK)
	{
		result = comm_error_result;
	}
	
	if(actualLength != NULL)
	{
		printf("RX: ");
		for(index_dbg = 0; index_dbg < *actualLength; index_dbg++)
		{
			printf("%02X ", message[index_dbg]);
		}
		printf("\n");
	}
	
	/*Reset interrupt flag as this timer is also used by other UART functions*/
	if(TIMER_SOFTWARE_interrupt_pending(timer_TX_RX))
	{
		TIMER_SOFTWARE_clear_interrupt(timer_TX_RX);
	}
	
	return result;
}

/*************************************************************************************************************************************************
	Function: 	UART1_flush
	Description: This function flushes the SW buffer (which is used in UART RX interrupt). Must not be called before UART1_init() and InterruptInit() functions
	Parameters: void
  Return value:
							TRUE = Flush operation was successfull
							FALSE = Flush operation failed
*************************************************************************************************************************************************/
bool_t UART1_flush(void)
{
	bool_t result;

	RingBufFlush(&uart1_ringbuff_rx);
	
	/*Check if SW ISR buffer is actually empty. This check is done for robustness*/
	result = RingBufEmpty(&uart1_ringbuff_rx);
	
	return result;
}
