/******************************************************************************/
/* SERIAL.C: Low Level Serial Routines                                        */
/******************************************************************************/
/* This file is part of the uVision/ARM development tools.                    */
/* Copyright (c) 2005-2006 Keil Software. All rights reserved.                */
/* This software may only be used under the terms of a valid, current,        */
/* end user licence from KEIL for a compatible version of KEIL software       */
/* development tools. Nothing else gives you the right to use this software.  */
/******************************************************************************/

#include <LPC22xx.H>                     /* LPC21xx definitions               */
#include "serial.h"

#define CR     0x0D

void initSerial(void)	// initialize the serial interface 
{
  PINSEL0 = 0x00000005;           
  U0LCR = 0x83;                   
  U0DLL = 20;                     // BAUD = 128.000 bps (max)
  U0LCR = 0x03;
  U0FCR = 0x01;	// fifo enable


  //sendchar('x');
}

int sendchar (int ch)  
{                 /* Write character to Serial Port    */

  	while (!(U0LSR & 0x20));
  	return (U0THR = ch);
}
  



void sendString(char *p)
{
	while (*p)
	{
		sendchar(*p);
		p++;
	}
}


int getkey (void)  {                     /* Read character from Serial Port   */

  	while (!(U0LSR & 0x01));
   	return (U0RBR);
}
