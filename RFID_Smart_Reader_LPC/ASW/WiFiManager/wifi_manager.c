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
typedef enum init_sequence_en
{
	AT_EN,
	STATION_MODE_EN,
	CONNECT_WI_FI_EN,
	ALLOW_MULTIPLE_CONNECTIONS_EN,
	LIMIT_TO_1_CONNECTION,
	OPEN_TCP_SERVER,
	GET_STATUS_AND_IP,
	END_OF_SEQUENCE
}init_sequence_en;

static const AT_Command_st wifi_init_config[] = {
																									[AT_EN] = {"AT\r\n", (uint16_t)4, (uint8_t)2},
																									[STATION_MODE_EN] = {"AT+CWMODE=1\r\n", (uint16_t)13, (uint8_t)2}, /*Set ESP to Station mode*/
																									//[CONNECT_WI_FI_EN] = {"AT+CWJAP=\"TP-Link_4FE4\",\"00773126\"\r\n", (uint16_t)36, (uint8_t)4},
																									[CONNECT_WI_FI_EN] = {"AT+CWJAP=\"DIGI-02349788\",\"gy3cUath\"\r\n", (uint16_t)37, (uint8_t)2},
																									[ALLOW_MULTIPLE_CONNECTIONS_EN] = {"AT+CIPMUX=1\r\n", (uint16_t)13, (uint8_t)2}, /*Allow multiple connections in order to put module on TCP Server mode*/
																									[LIMIT_TO_1_CONNECTION] = {"AT+CIPSERVERMAXCONN=1\r\n", (uint16_t)23, (uint8_t)2},
																									[OPEN_TCP_SERVER] = {"AT+CIPSERVER=1,8080\r\n", (uint16_t)21, (uint8_t)2},
																									[GET_STATUS_AND_IP] = {"AT+CIPSTA?\r\n", (uint16_t)12, (uint8_t)4}
																								};

static AT_response_st wifi_response_buffer[5];
static bool manager_init_ready_flag = false;																							

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
	init_sequence_en init_seq_index;
	command_frame_status_t message_status = WI_FI_COMMAND_NOK;
	wifi_module_state_st module_state;
	
	wifi_utils_Init();
	
	/*Pin P0.25 will be used (as output) for HW reset of ESP32-C3 wi-fi module*/
	IO0DIR |= ((uint32_t)1U << ESP_RESET_PIN_NUM_U8);
	
	/*Pin P0.15 will be used for GPIO Handsake polling, by default is configured as input*/
	
	WifiManager_Perform_HW_Reset();
	
	message_status = Wait_For_HIGH_Transition();
	if(WI_FI_COMMAND_OK == message_status)
	{
		message_status = Read_Ready_Status();
		if(WI_FI_COMMAND_OK == message_status)
		{
			for(init_seq_index = AT_EN; init_seq_index < END_OF_SEQUENCE; init_seq_index++)
			{
				module_state = Get_Module_Current_State();
				
//				/*Check if current step is Connecting to Wi-Fi and if it really is necessary*/
//				if((true == module_state.wifi_connected) && (init_seq_index == CONNECT_WI_FI_EN))
//				{
//					/*Skip Wi Fi connect sequence if already connected*/
//					init_seq_index++;
//				}
				
				message_status = Send_ESP_Command(wifi_init_config[init_seq_index], wifi_response_buffer);
				
				if(WI_FI_COMMAND_NOK == message_status)
				{
					/*If one command was wrong, do not continue*/
					break;
				}
			}
		}
	}
	
	module_state = Get_Module_Current_State();
	
	if((WI_FI_COMMAND_OK == message_status) && (true == module_state.HW_module_init) && (true == module_state.wifi_connected) && true == module_state.wifi_got_ip)
	{
		/*Mark Wi-Fi module initialization as OK. WiFi Manager will not start otherwise*/
		manager_init_ready_flag = true;
		#ifdef WI_FI_DEBUG
		printf("\nWI FI MODULE INIT OK\n");
		#endif /*WI_FI_DEBUG*/
	}
	else
	{
		manager_init_ready_flag = false;
		LP_Set_StayAwake(FUNC_WIFI_MANAGER, false);
		#ifdef WI_FI_DEBUG
		printf("\nWI FI MODULE INIT  NOT OK\n");
		#endif /*WI_FI_DEBUG*/
	}
}

void Wifi_Manager(void)
{
	if(manager_init_ready_flag == false)
	{
		LP_Set_StayAwake(FUNC_WIFI_MANAGER, false);
	}
	else
	{
	
	}
}
