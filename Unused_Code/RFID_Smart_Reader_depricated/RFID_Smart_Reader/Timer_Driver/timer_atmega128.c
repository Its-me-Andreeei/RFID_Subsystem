/*
 * timer_atmega129.c
 *
 * Created: 26.11.2023 13:24:41
 *  Author: Karolyi Andrei - Cristian
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#include "../UART_driver/UART_Driver.h"
#include "timer_atmega128.h"
#include "../PlatformTypes.h"

typedef struct timer {
	TIMER_STATUS_EN status_timer;
	TIMER_START_REASON_EN start_reason;
	}Timer;
	
	
static Timer timer1 = (Timer){TIMER_IDLE, TIMER_NO_REASON};
static Timer timer3 = (Timer){TIMER_IDLE, TIMER_NO_REASON};

static inline TIMER_STATUS_EN check_timer_compare_done(TIMER_NO_EN timer_no)
{
	TIMER_STATUS_EN result;
	if(TIMER1 == timer_no)
	{
		if((u8)TIFR & ((u8)1U << (u8)OCF1A))
		{
			timer1.status_timer = TIMER_FINISHED;
		}
		result = timer1.status_timer;
	}
	else if( TIMER3 == timer_no)
	{
		if((u8)ETIFR & ((u8)1U << (u8)OCF3A))
		{
			timer3.status_timer = TIMER_FINISHED;
		}
		result = timer3.status_timer;
	}
	else
	{
		result = TIMER_UNKNOWN_STATE;
		#ifdef DEBUG
		UART1_SendMessage_DEBUG("check_timer_compare_done: Unknown timer number");
		#endif
	}
	return result;
}

static void enable_timer1(const u32 value_ms)
{
	u16 ocr1a_value = 0u;
	if((TIMER_NO_REASON != timer1.start_reason) && (TIMER_IN_PROGRESS != timer1.status_timer))
	{
		timer1.status_timer = TIMER_IN_PROGRESS;
		
		/*Reset counter value so we start from "0"*/
		TIFR = (u8)0x00U;
		TCNT1H = (u8)0x00U; /*This is done for robustness*/
		TCNT1L = (u8)0x00U; /*This is done for robustness*/
		
		#ifdef DEBUG
		if ( (0xFFFF0000u & value_ms) !=0u) /*For moment only values up to 16 bits supported*/
		{
			char text[100];
			sprintf(text, "enable_timer1: %lu is too large (bigger then 16 bits)", value_ms);
			UART1_SendMessage_DEBUG(text);
		}
		#endif
		
		TCCR1A = (u8)0x00U; /*This is done for robustness*/
		TCCR1B = ((u8)1<<(u8)WGM12); /*Set CTC mode*/
		
		ocr1a_value = (value_ms * F_CPU) / 1024u;
		OCR1AH = (u8)((ocr1a_value >> 8u) & 0xFFu);
		OCR1AL = (u8)(ocr1a_value & 0xFFu);
		TCCR1B = (u8)((u8)1 << (u8)CS12) | ((u8)1 << (u8)CS10); /*Prescaler is 1024 according to maximum value possible for timeout (6s)*/
	}
	#ifdef DEBUG
	else
	{
		if(TIMER_IN_PROGRESS == timer1.status_timer)
		{
			UART1_SendMessage_DEBUG("enable_timer1: Timer1 is already running");
		}
		if(TIMER_NO_REASON == timer1.start_reason)
		{
			UART1_SendMessage_DEBUG("enable_timer1: Timer1 has no start reason set");
		}
	}
	#endif
}

static void enable_timer3(const u32 value_ms)
{
	u16 ocr3a_value = 0u;
	if((TIMER_NO_REASON != timer3.start_reason) && (TIMER_IN_PROGRESS != timer3.status_timer))
	{
		timer3.status_timer = TIMER_IN_PROGRESS;
		
		/*Reset counter value so we start from "0"*/
		ETIFR = (u8)0x00U;
		TCNT3H = (u8)0x00U; /*This is done for robustness*/
		TCNT3L = (u8)0x00U; /*This is done for robustness*/
		
		#ifdef DEBUG
		if ( (0xFFFF0000u & value_ms) !=0u) /*For moment only values up to 16 bits supported*/
		{
			char text[100];
			sprintf(text, "enable_timer3: %lu is too large (bigger then 16 bits)", value_ms);
			UART1_SendMessage_DEBUG(text);
		}
		#endif
		
		TCCR3A = (u8)0x00U; /*This is done for robustness*/
		TCCR3B = ((u8)1<<(u8)WGM32); /*Set CTC mode*/
		
		ocr3a_value = (value_ms * F_CPU) / 1024u;
		OCR3AH = (u8)((ocr3a_value >> 8u) & 0xFFu);
		OCR3AL = (u8)(ocr3a_value & 0xFFu);
		TCCR3B = (u8)((u8)1 << (u8)CS32) | ((u8)1 << (u8)CS30); /*Prescaler is 1024 according to maximum value possible for timeout (6s)*/
	}
	#ifdef DEBUG
	else
	{
		if(TIMER_IN_PROGRESS == timer3.status_timer)
		{
			UART1_SendMessage_DEBUG("enable_timer3: Timer3 is already running");
		}
		if(TIMER_NO_REASON == timer3.start_reason)
		{
			UART1_SendMessage_DEBUG("enable_timer3: Timer3 has no start reason set");
		}
	}
	#endif
}

 void start_timer(const TIMER_NO_EN timer_no, const u32 value_ms) {
	 if(TIMER1 == timer_no)
	 {
		 enable_timer1(value_ms);
	 }
	 else if(TIMER3 == timer_no)
	 {
		 enable_timer3(value_ms);
	 }
	 #ifdef DEBUG
	 else
	 {
		 UART1_SendMessage_DEBUG("start_timer: Wrong timer number given");
	 }
	 #endif
 }
 
TIMER_STATUS_EN get_timer_status(const TIMER_NO_EN timer_no)
{
	TIMER_STATUS_EN result;
	
	if(TIMER1 == timer_no)
	{
		if(TIMER_FINISHED == check_timer_compare_done(timer_no))
		{
			TCCR1B = (u8)0x00U; /*Stop the timer*/
			TCNT1H = (u8)0x00U;/*Reset acutal value from the counter*/
			TCNT1L = (u8)0x00U;
			TIFR |= ((u8)1U << (u8)OCF1A); /*Clear OCF1A flag*/
		}
		result = timer1.status_timer;
	}
	else if(TIMER3 == timer_no)
	{
		if(TIMER_FINISHED == check_timer_compare_done(timer_no))
		{
			TCCR3B = (u8)0x00U; /*Stop the timer*/
			TCNT3H = (u8)0x00U;/*Reset acutal value from the counter*/
			TCNT3L = (u8)0x00U;
			ETIFR |= ((u8)1U << (u8)OCF3A); /*Clear OCF3A flag*/
		}
		result = timer1.status_timer;
	}
	else
	{
		result = TIMER_UNKNOWN_STATE;
		#ifdef DEBUG
		UART1_SendMessage_DEBUG("get_timer_status: Timer number is wrong");
		#endif
	}
	
	return result;
}

void disable_timer(const TIMER_NO_EN timer_no)
{
	if(TIMER1 == timer_no)
	{
		timer1.status_timer = TIMER_IDLE;
		timer1.start_reason = TIMER_NO_REASON;
		TCCR1B = (u8)0x00U; /*Stop the timer*/
		TCNT1H = (u8)0x00U;/*Reset acutal value from the counter*/
		TCNT1L = (u8)0x00U;
		TIFR |= ((u8)1U << (u8)OCF1A); /*Clear OCF1A flag*/
	}
	else if(TIMER3 == timer_no)
	{
		timer3.status_timer = TIMER_IDLE;
		timer3.start_reason = TIMER_NO_REASON;
		TCCR3B = (u8)0x00U; /*Stop the timer*/
		TCNT3H = (u8)0x00U;/*Reset acutal value from the counter*/
		TCNT3L = (u8)0x00U;
		ETIFR |= ((u8)1U << (u8)OCF3A); /*Clear OCF1A flag*/
	}
	#ifdef DEBUG
	else
	{
		UART1_SendMessage_DEBUG("disable_timer: Wrong timer number given");
	}
	#endif
	
}

void set_start_reason_timer(const TIMER_NO_EN timer_no, const TIMER_START_REASON_EN reason)
{
	if(TIMER1 == timer_no)
	{
		timer1.start_reason = reason;
	}
	else if (TIMER3 == timer_no)
	{
		timer3.start_reason = reason;
	}
	#ifdef DEBUG
	else
	{
		UART1_SendMessage_DEBUG("set_start_reason_timer: Unknown timer number given");
	}
	#endif
}

TIMER_START_REASON_EN get_start_reason_timer(const TIMER_NO_EN timer_no)
{
	TIMER_START_REASON_EN reason;
	if(TIMER1 == timer_no)
	{
		reason = timer1.start_reason;
	}
	else if(TIMER3 == timer_no)
	{
		reason = timer3.start_reason;
	}
	else
	{
		reason = TIMER_REASON_UNKNOWN;
		#ifdef DEBUG
		UART1_SendMessage_DEBUG("get_start_reason_timer1: Unknown timer number given");
		#endif
	}
	return reason;
}
