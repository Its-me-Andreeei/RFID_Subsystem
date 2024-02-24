#ifndef __UART1_H
#define __UART1_H

#include <stdint.h>

#define TRUE ((uint8_t)1U)
#define FALSE ((uint8_t)0U)

typedef uint8_t bool_t;

typedef enum UART_TX_RX_Status_en
{
	RETURN_OK,
	TIMEOUT_ERROR,
	COMMUNICATION_ERROR,
	RETURN_NOK /*This is a generic negative response (e.g. usage : the actual response is not the expected one, in case of PINGs for example*/
}UART_TX_RX_Status_en;

void UART1_Init(void);
bool_t UART1_flush(void);
UART_TX_RX_Status_en UART1_sendbuffer(const uint8_t *buffer, const uint32_t size, const uint32_t timeoutMs);
UART_TX_RX_Status_en UART1_receivebuffer(uint8_t* message, uint32_t expectedLength, uint32_t* actualLength, const uint32_t timeoutMs);
uint8_t UART1_get_CommErrors(void);
UART_TX_RX_Status_en UART1_send_reveice_PING(void);
#endif
