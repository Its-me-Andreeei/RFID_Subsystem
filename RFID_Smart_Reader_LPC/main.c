#include <lpc22xx.h>
#include <stdio.h>
#include "hal/timer.h"
#include "hal/uart0.h"
#include "utils/timer_software.h"
#include "hal/ISR_manager.h"
#include "mercuryapi-1.37.1.44/c/src/api/tm_reader.h"
#include "hal/uart1.h"
#include "ASW/ReaderManager/reader_manager.h"

void Module_Reset()
{
	IO0CLR = 1 << 23;
	TIMER_SOFTWARE_Wait(1000);
	IO0SET = 1 << 23;
	TIMER_SOFTWARE_Wait(1000);
}

int main(void)
{	 
	timer_software_handler_t timer1;
	IO0DIR |= (uint8_t)1 << (uint8_t)23;
	IO0CLR = (uint8_t)1 << (uint8_t)23;
	
	TIMER_SOFTWARE_init();
	TIMER_Init(); /*This is the HW timer*/
	UART0_Init(); /*Debugging port*/
	UART1_Init();
	printf("Main");

	InitInterrupt();
	Reader_Reset();
	/*------------------------------------*/
	ReaderManagerInit();
	/*------------------------------------*/

	timer1 = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer1, MODE_0, 5000, 1);
	TIMER_SOFTWARE_reset_timer(timer1);
	/*------------------------------------*/

	TIMER_SOFTWARE_start_timer(timer1);
	while(1)
	{
		Reader_Manager();
	}
	return 0;
}

