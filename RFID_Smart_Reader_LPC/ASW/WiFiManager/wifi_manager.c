#include <lpc22xx.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "./wifi_Manager.h"
#include "./../../hal/spi.h"



typedef enum wifi_manager_request_t
{
	WIFI_REQUEST_CONNECT,
	WIFI_REQUEST_CHECK_MODULE
}wifi_manager_request_t;


void Wifi_Manager(void)
{
	static const unsigned char command[5] = "AT\r\n";
	static bool onetime = false;
	
	if((onetime == false) && (spi0_get_request_status() == NO_REQUEST))
	{
		spi0_set_request(command, 4);
		onetime = true;
	}
	if(spi0_get_request_status() == REQUEST_IN_PROGRESS)
	{
		spi0_process_request();
	}
	if(spi0_get_request_status() == REQUEST_COMPLETED)
	{
		uint8_t output[20];
		uint16_t length;
		uint16_t index;
		spi0_get_response(output, &length);
		
		for(index = 0; index <length; index++)
		{
			printf("%c ", output[index]);
		}
		printf("\n");
	}
}
