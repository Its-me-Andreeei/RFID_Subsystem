#ifndef __UART1_H
#define __UART1_H

#include "../PlatformTypes.h"

typedef enum UART_TX_RX_Status_en
{
	RETURN_OK,
	TIMEOUT_ERROR,
	COMMUNICATION_ERROR,
	RETURN_NOK /*This is a generic negative response (e.g. usage : the actual response is not the expected one, in case of PINGs for example*/
}UART_TX_RX_Status_en;

void UART1_Init(void);
bool UART1_flush(void);
UART_TX_RX_Status_en UART1_sendbuffer(const u8 *buffer, const u32 size, const u32 timeoutMs);
UART_TX_RX_Status_en UART1_receivebuffer(u8* message, u32 expectedLength, u32* actualLength, const u32 timeoutMs);
u8 UART1_get_CommErrors(void);
UART_TX_RX_Status_en UART1_send_reveice_PING(void);
#endif
