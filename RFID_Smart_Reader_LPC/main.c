#include <lpc22xx.h>
#include <stdio.h>
#include "hal/timer.h"
#include "hal/uart0.h"
#include "utils/timer_software.h"
#include "hal/ISR_manager.h"
#include "hal/uart1.h"
#include "hal/i2c.h"
#include "hal/spi.h"

#include "ASW/ReaderManager/reader_manager.h"
#include "ASW/HostCommManager/HostCommManager.h"
#include "ASW/LP_Mode_Manager/LP_Mode_Manager.h"
#include "ASW/WiFiManager/wifi_manager.h"

int main(void)
{	
	#define READER_HW_RESET_PIN_U8 ((uint8_t) 23U)
	IO0DIR |= (uint32_t)1 << READER_HW_RESET_PIN_U8;
	IO0CLR = (uint32_t)1 << READER_HW_RESET_PIN_U8;
	
	TIMER_SOFTWARE_init(); /*This is the SW timer*/
	TIMER_Init(); /*This is the HW timer*/
	UART0_Init(); /*Debugging port*/
	UART1_Init();
	I2C_init();
	SPI0_init();
	printf("Main");

	InitInterrupt();
	/*------------------------------------*/
	LP_Mode_Manager_Init();
	WifiManager_Init();
	Reader_HW_Reset();
	ReaderManagerInit();
	HostComm_Manager_Init();
	/*------------------------------------*/
	
	while(1)
	{	
		Reader_Manager();
		HostComm_Manager();
		Wifi_Manager();
		//LP_Mode_Manager();
	}
	return 0;
}

