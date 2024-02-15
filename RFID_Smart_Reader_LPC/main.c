#include <lpc22xx.h>
#include <stdio.h>
#include "hal/timer.h"
#include "hal/uart0.h"
#include "utils/timer_software.h"
#include "hal/ISR_manager.h"
#include "mercuryapi-1.37.1.44/c/src/api/tm_reader.h"
#include "hal/uart1.h"
#include "ASW/ReaderManager/reader_manager.h"


int main(void)
{	 
	IO0DIR |= (uint8_t)1 << (uint8_t)23;
	IO0CLR = (uint8_t)1 << (uint8_t)23;
	
	TIMER_SOFTWARE_init();
	TIMER_Init(); /*This is the HW timer*/
	UART0_Init(); /*Debugging port*/
	UART1_Init();
	printf("Main");

	InitInterrupt();
	/*------------------------------------*/
	Reader_HW_Reset();
	ReaderManagerInit();
	/*------------------------------------*/

	while(1)
	{
		Reader_Manager();
	}
	return 0;
}

