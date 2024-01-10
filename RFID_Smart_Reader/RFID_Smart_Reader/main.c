/*
 * RFID_Smart_Reader.c
 *
 * Created: 04.11.2023 16:46:36
 * Author : Karolyi Andrei - Cristian
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include "mercuryapi-1.37.1.44/c/src/api/tm_reader.h"
#include "UART_driver/UART_Driver.h"
#include "Timer_Driver/timer_atmega128.h"

/*TBD: Debug variable: To be removed after testing is done*/
static volatile u8 readError = 0;

int main(void)
{
	#ifdef DEBUG
    UART_init(9600, DEBUG_INTERFACE); /*placed here in order to open UART for DEBUGGING Purpose*/
	//UART1_SendMessage_DEBUG("DEBUG: ON");
	#endif
	
	TMR_Reader reader;
	TMR_Status read_status;
	int32_t readTagCount;
	TMR_TagReadData readData;
	
	cli();
	TMR_SR_SerialTransportDummyInit(&reader.u.serialReader.transport, NULL, NULL); /*This will set the functions from implemented driver*/
	/*Region is right now hardcoded inside init function*/
	TMR_SR_SerialReader_init(&reader);
	TMR_connect(&reader); /*Enable UART between MCU and reader*/
 	(void)TMR_startReading(&reader); /* Set read flag: on*/
	
	
    while (1) 
    {
		read_status = TMR_read(&reader, 3000, &readTagCount);
		if(TMR_SUCCESS != read_status)
		{
			readError++;
		}
		if(TMR_ERROR_NO_TAGS != TMR_hasMoreTags(&reader))
		{
			TMR_getNextTag(&reader, &readData);
			asm("NOP");
		}
    }
	
	return 0;
}

