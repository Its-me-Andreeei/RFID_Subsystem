/*
 * timer_atmega128.h
 *
 * Created: 26.11.2023 13:24:57
 *  Author: Nitro5
 */ 


#ifndef TIMER_ATMEGA128_H_
#define TIMER_ATMEGA128_H_
#include "../PlatformTypes.h"

typedef enum {
	TIMER_IDLE = 0,
	TIMER_IN_PROGRESS = 1,
	TIMER_FINISHED = 2,
	TIMER_UNKNOWN_STATE /*This can occour if timer is unknown*/
	}TIMER_STATUS_EN;

typedef enum {
	TIMER_NO_REASON = 0,
	TIMER_UART0_TX = 1,
	TIMER_UART0_RX = 2,
	TIMER_REASON_UNKNOWN
	}TIMER_START_REASON_EN;
	
typedef enum {
	TIMER1 = 0,
	TIMER3 = 1,
	TIMER_UNKNOWN
	}TIMER_NO_EN;
void start_timer(const TIMER_NO_EN timer_no, const u32 value_ms);
TIMER_STATUS_EN get_timer_status(const TIMER_NO_EN timer_no);
void disable_timer(const TIMER_NO_EN timer_no);

void set_start_reason_timer(const TIMER_NO_EN timer_no, const TIMER_START_REASON_EN reason);
TIMER_START_REASON_EN get_start_reason_timer(const TIMER_NO_EN timer_no);
#endif /* TIMER_ATMEGA128_H_ */