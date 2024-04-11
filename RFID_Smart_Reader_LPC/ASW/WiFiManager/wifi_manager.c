#include <lpc22xx.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "./../../utils/timer_software.h"
#include "./wifi_Manager.h"
#include "./wifi_utils.h"
#include "./../../hal/spi.h"
#include "./../LP_Mode_Manager/LP_Mode_Manager.h"

#define ESP_RESET_PIN_NUM_U8 ((uint8_t)25U)

static const AT_Command_st wifi_init_config[4] = {
																									{"AT\r\n", (uint16_t)4, (uint8_t)2},
																									{"AT+CWMODE=1\r\n", (uint16_t)13, (uint8_t)2}, /*Set ESP to Station mode*/
																									{"AT+CWJAP=\"TP-Link_4FE4\",\"00773126\"\r\n", (uint16_t)36, (uint8_t)4},
																									{"AT+CIPSTA?\r\n", (uint16_t)12, (uint8_t)3}
																								};

static AT_response_st wifi_response_buffer[5];
static bool init_ready_flag = false;																							

void WifiManager_Perform_HW_Reset(void)
{
	IO0SET |= ((uint8_t)1U << ESP_RESET_PIN_NUM_U8);
	TIMER_SOFTWARE_Wait(1000);
	IO0CLR |= ((uint8_t)1U << ESP_RESET_PIN_NUM_U8);
	TIMER_SOFTWARE_Wait(1000);
	IO0SET |= ((uint8_t)1U << ESP_RESET_PIN_NUM_U8);
	TIMER_SOFTWARE_Wait(1000);
}

void WifiManager_Init(void)
{
	uint8_t init_sequence_length;
	uint8_t init_seq_index;
	command_frame_status_t message_status = WI_FI_COMMAND_NOK;
	
	wifi_utils_Init();
	
	/*Pin P0.25 will be used (as output) for HW reset of ESP32-C3 wi-fi module*/
	IO0DIR |= ((uint32_t)1U << ESP_RESET_PIN_NUM_U8);
	
	/*Pin P0.15 will be used for GPIO Handsake polling, by default is configured as input*/
	
	init_sequence_length = sizeof(wifi_init_config)/sizeof(AT_Command_st);
	
	WifiManager_Perform_HW_Reset();
	
	message_status = Wait_For_HIGH_Transition();
	if(WI_FI_COMMAND_OK == message_status)
	{
		message_status = Read_Ready_Status();
		if(WI_FI_COMMAND_OK == message_status)
		{
			for(init_seq_index = 0; init_seq_index < init_sequence_length; init_seq_index++)
			{
				if(init_seq_index == 3)
					__nop();
				message_status = Send_ESP_Command(wifi_init_config[init_seq_index], wifi_response_buffer);
				if(WI_FI_COMMAND_NOK == message_status)
				{
					/*If one command was wrong, do not continue*/
					break;
				}
			}
		}
	}
	
	if(WI_FI_COMMAND_OK == message_status)
	{
		/*Mark Wi-Fi module initialization as OK. WiFi Manager will not start otherwise*/
		init_ready_flag = true;
		#ifdef WI_FI_DEBUG
		printf("\nWI FI MODULE INIT OK\n");
		#endif /*WI_FI_DEBUG*/
	}
	else
	{
		init_ready_flag = false;
		LP_Set_StayAwake(FUNC_WIFI_MANAGER, false);
		#ifdef WI_FI_DEBUG
		printf("\nWI FI MODULE INIT  NOT OK\n");
		#endif /*WI_FI_DEBUG*/
	}
}

void Wifi_Manager(void)
{
	if(init_ready_flag == false)
	{
		LP_Set_StayAwake(FUNC_WIFI_MANAGER, false);
	}
	else
	{
	
	}
}
