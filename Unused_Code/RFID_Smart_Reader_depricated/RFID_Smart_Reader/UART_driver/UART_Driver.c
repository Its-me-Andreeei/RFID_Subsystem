/*
 * UART_Driver.c
 *
 * Created: 18.11.2023 20:17:28
 *  Author: Andrei Karolyi
 */ 

#include "UART_Driver.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h> 
#include <stdio.h>

 #include "../PlatformTypes.h"
 #include "../Timer_Driver/timer_atmega128.h"

/*-----------------------UART 0 Macros---------------------------------------------------------------------------------------------*/
#define UART0_isBufferEmpty() (((u8)UCSR0A & ((u8)1U << (u8)UDRE0)))
#define UART0_isTxComplete() (((u8)UCSR0A & (((u8)1U << (u8)TXC0) | ((u8)1U  << (u8)UDRE0))))
#define UART0_isCommErrorDetected() ((((u8)UCSR0A & ((u8)1U << (u8)FE0)) != 0) || (((u8)UCSR0A & ((u8)1U << (u8)DOR0))))
#define UART0_isRxComplete() ((u8)UCSR0A & ((u8)1U << (u8)RXC0))
#define MAX_RX_BUFFER_SIZE ((u8)255u)
/*-----------------------UART 1 Macros---------------------------------------------------------------------------------------------*/
#define UART1_isBufferEmpty() (((u8)UCSR1A & ((u8)1U << (u8)UDRE1)))

/*---------------------------------------------------------------------------------------------------------------------------------*/
 
 
 static volatile u8 ISR_Buffer[MAX_RX_BUFFER_SIZE];
 static volatile u16 ISR_end_index;
 
 ISR(USART0_RX_vect) 
{
	if(ISR_end_index >= MAX_RX_BUFFER_SIZE)
		ISR_end_index = (u8)0U;
	ISR_Buffer[ISR_end_index] = UDR0;
	while((u8)UCSR0A & ((u8)1U << (u8)RXC0))	
		ISR_Buffer[ISR_end_index] = (u8)UDR0;
	ISR_end_index++;
}

void UART_init(const unsigned long baud, UART_INIT_MODE_EN uart_mode) {
	if(DEBUG_INTERFACE == uart_mode)
	{
		DDRD |= ((u8)1U << (u8)PD3);
		UBRR1L = F_CPU/16/baud-1;
		UCSR1C = ((u8)1U << (u8)UCSZ11) | ((u8)1U << (u8)UCSZ10);
		UCSR1B = ((u8)1U << (u8)TXEN) | ((u8)1U << (u8)RXEN);
	}
	else if(READER_INTERFACE == uart_mode) 
	{
		DDRE |= ((u8)1U << (u8)PE1); 
		UBRR0L = F_CPU/16/baud-1;
		UCSR0C = ((u8)1U << (u8)UCSZ01) | ((u8)1U << (u8)UCSZ00);
		UCSR0B = ((u8)1U << (u8)TXEN) | ((u8)1U << (u8)RXEN);
	}
}

TMR_Status UART0_change_BAUD(const u32 baud) 
{
	TMR_Status result_status = TMR_ERROR_INVALID;
	if(((u32)9600u == baud) || ((u32)115200u == baud) || ((u32)921600u == baud) || ((u32)19200u == baud) || ((u32)38400u == baud) || ((u32)57600u == baud) || ((u32)230400u == baud) || ((u32)460800u == baud))
	{
		result_status = TMR_SUCCESS;
	}
	
	UCSR0B = (u8)0x00u; /*Temporary disable UART in order to set the new BAUD*/
	UBRR0L = F_CPU/16/baud-1;
	UCSR0B = ((u8)1U << (u8)TXEN) | ((u8)1U << (u8)RXEN); /* Enable UART in order to set the new BAUD*/
	
	return result_status;
}

void UART1_SendMessage_DEBUG(const char *text)
{
	uint8 i;
	
	for(i = (u8)0U; i <  (u8)strlen(text); i++)
	{
		while(FALSE == UART1_isBufferEmpty()) {} /*Wait for buffer to become empty*/
		if(FALSE != UART1_isBufferEmpty()) {
			UDR1 = (unsigned char)text[i];
		}
	}
}
 
TMR_Status UART0_SendMessage(const u8 *message, const u32 length, const u32 timeout_ms)
{
	
	u32 i = (u32)0;
	TMR_Status result_status = TMR_SUCCESS;
	if(TIMER_NO_REASON == get_start_reason_timer(TIMER1))
	{
		if((TIMER_IN_PROGRESS == get_timer_status(TIMER1)) || (TIMER_FINISHED == get_timer_status(TIMER1)))
		{
			disable_timer(TIMER1);
		}
		sei();
		UCSR0B |= ((u8)1U << (u8)RXCIE0);
		set_start_reason_timer(TIMER1, TIMER_UART0_TX);
 		start_timer(TIMER1, timeout_ms);
		for(i = (u32)0U; i <  length; i++)
		{
			/*Wait for buffer to become empty*/
			while(FALSE == UART0_isBufferEmpty()) 
			{
				/*if(TIMER_FINISHED == get_timer_status())
				{
					disable_timer();
					result_status = TMR_ERROR_TIMEOUT;
					break;
				}*/
			} 
		
			/*Check if Buffer is finally empty*/
			if(FALSE != UART0_isBufferEmpty()) 
			{
				/*Firstly we have to check for timer*/
				if(TIMER_FINISHED == get_timer_status(TIMER1))
				{
					disable_timer(TIMER1);
					result_status = TMR_ERROR_TIMEOUT;
					break;
				}
				else
				{
					UDR0 = (u8)message[i];
					while(FALSE == UART0_isTxComplete()) /*Wait for current byte to be sent*/
					{
						/*Do nothing*/
					}
					if(FALSE != UART0_isTxComplete())
					{
						UCSR0A |= ((u8)1U << (u8)TXC0); /*Clear the TX Flag*/
					}
				}
			}
		}
	
		if(TMR_ERROR_TIMEOUT != result_status)
		{
			if(FALSE != UART0_isCommErrorDetected())
			{
				result_status = TMR_ERROR_INVALID_VALUE;
			}
		}
	
		if((TIMER_IN_PROGRESS == get_timer_status(TIMER1)) || (TIMER_FINISHED == get_timer_status(TIMER1)))
		{
			disable_timer(TIMER1);
		}
	
		if(FALSE != UART0_isTxComplete())
		{
			UCSR0A |= ((u8)1U << (u8)TXC0); /*Clear the TX Flag*/
		}
	}
	else /*Timer reason not OK*/
	{
		result_status = TMR_ERROR_INVALID_VALUE;
		#ifdef DEBUG
		UART1_SendMessage_DEBUG("UART0_SendMessage: Timer1 must be deactivated first");
		#endif // DEBUG
	}
	return result_status;
}

void UART0_flush(void) {
	u8 tmp_buffer_dummy;
	
	/*Buffer is not empty, so we must clear it*/
	if(FALSE == UART0_isBufferEmpty())
	{
		while((FALSE != UART0_isRxComplete()) || (FALSE == UART0_isBufferEmpty()))
		{
			tmp_buffer_dummy = (u8)UDR0; /*Reading buffer will eventually clear RXC0 flag*/
		}
		(void)tmp_buffer_dummy; /*Dummy code with no functional impact. This was added in order to erase unused parameter warning*/

		/*Disabling RX will clear the buffer. This was added for robustness*/
		UCSR0B = (u8)UCSR0B & (~((u8)1U << (u8)RXEN0)); /*Disable RX of UART0*/
		UCSR0B = (u8)UCSR0B | ((u8)1U << (u8)RXEN0);/*Enable RX of UART0*/
	}
}
void UART0_DeInit(void) 
{
	UART0_flush();
	UCSR0B = (u8)0x00U; /*This will also disable RXEN and TXEN, so UART will be closed*/
	UCSR0A = (u8)0x00U;
	UCSR0C = (u8)0x00U;
	
	/* Even if clearing baud was not necessary to disable UART, it's done for robustness. Init function will set it again*/
	UBRR0H = (u8)0x00U;
	UBRR0L = (u8)0x00U;
	
	/*TBD: Will be removed later*/
	#ifdef DEBUG
	char tmp_debug[5]; /*Registers format as hex 0xXY*/
	if((u8)0x00U != (u8)UCSR0A)
	{
		UART1_SendMessage_DEBUG("UCSR0A:");
		sprintf(tmp_debug, "0x%X", (u8)UCSR0A);
		UART1_SendMessage_DEBUG(tmp_debug);
	}
	if((u8)0x00U != (u8)UCSR0B)
	{
		UART1_SendMessage_DEBUG("UCSR0B:");
		sprintf(tmp_debug, "0x%X", (u8)UCSR0B);
		UART1_SendMessage_DEBUG(tmp_debug);
	}
	if((u8)0x00U != (u8)UCSR0C)
	{
		UART1_SendMessage_DEBUG("UCSR0C:");
		sprintf(tmp_debug, "0x%X", (u8)UCSR0C);
		UART1_SendMessage_DEBUG(tmp_debug);
	} 
	
	#endif
}

TMR_Status UART0_ReceiveMessage(u8* message, const u32 length, u32* messageLength, const u32 timeoutMs) {
	TMR_Status result_status = TMR_SUCCESS;
	
	if((messageLength == NULL) || (message == NULL))
	{
		result_status = TMR_ERROR_INVALID_VALUE;
		#ifdef _DEBUG
		UART1_SendMessage_DEBUG("UART0_ReceiveMessage: Null argument");
		#endif // _DEBUG
	}
	else if(TIMER_NO_REASON == get_start_reason_timer(TIMER3))
	{
		set_start_reason_timer(TIMER3, TIMER_UART0_RX);
		start_timer(TIMER3, timeoutMs);
		
		while(TIMER_FINISHED != get_timer_status(TIMER3))
		{
			*messageLength = ISR_end_index;
			if(*messageLength == length)
			{
				disable_timer(TIMER3);
				break;
			}
		}
		
		if(length == *messageLength)
		{
			for(u8 i = (u8)0U; i < length; i++)
			{
				message[i] = ISR_Buffer[i];
				ISR_Buffer[i] = (u8)0U; /*After setting value in Official buffer, reset it*/
			}
			ISR_end_index = (u8)0U; /*Clear index of ISR buffer, for a new transmission*/
		}
		else
		{
			result_status = TMR_ERROR_TIMEOUT;
		}
	}
	else
	{
		result_status = TMR_ERROR_INVALID_VALUE;
		#ifdef DEBUG
		UART1_SendMessage_DEBUG("UART0_ReceiveMessage: Timer3 is in use");
		#endif // DEBUG
	}
	return result_status;
}