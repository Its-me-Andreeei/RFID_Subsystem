#ifndef __SERIAL_H
#define __SERIAL_H

void initSerial(void);			// initializare interfata seriala
int sendchar (int ch);			// trimitere caracter pe seriala
int getkey (void);				// cititre caracter de pe seriala
void sendString(char *p);		// trimitere sir de caractere pe seriala


#endif
