#ifndef __UART1_H
#define __UART1_H

#include <stdint.h>

#define TRUE ((uint8_t)1U)
#define FALSE ((uint8_t)0U)

typedef uint8_t bool_t;
	
void UART1_Init(void);
int UART1_sendchar(int ch);
bool_t UART1_flush(void);
bool_t UART1_sendbuffer(uint8_t *buffer, uint32_t size, const uint32_t timeoutMs);
bool_t UART1_receivebuffer(uint8_t* message, uint32_t expectedLength, uint32_t* actualLength, const uint32_t timeoutMs);
#endif
