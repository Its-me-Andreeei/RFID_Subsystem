/*
 * UART_Driver.h
 *
 * Created: 18.11.2023 20:17:49
 *  Author: Andrei Karolyi
 */ 


#ifndef UART_DRIVER_H_
#define UART_DRIVER_H_

#include "../PlatformTypes.h"
#include "..\mercuryapi-1.37.1.44\c\src\api\tmr_status.h"
typedef enum UART_INIT_MODE_EN {
	DEBUG_INTERFACE,
	READER_INTERFACE
	} UART_INIT_MODE_EN;

void UART_init(const unsigned long baud, enum UART_INIT_MODE_EN uart_mode);
void UART1_SendMessage_DEBUG(const char *text);

void UART0_flush(void);
void UART0_DeInit(void);
TMR_Status UART0_change_BAUD(const u32 baud);
TMR_Status UART0_SendMessage(const uint8 *message, const uint32 length, const uint32 timeout_ms);
TMR_Status UART0_ReceiveMessage(u8* message, const u32 length, u32* messageLength, const u32 timeoutMs);

#endif /* UART_DRIVER_H_ */