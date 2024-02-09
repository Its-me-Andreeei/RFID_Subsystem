#ifndef __SERIAL_H
#define __SERIAL_H

void sendchar_UART0 (int ch);
void sendchar_UART1 (int ch); 
void init_uart01(void);
void UART_cross(void);

#endif
