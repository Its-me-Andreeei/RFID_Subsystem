#include <lpc22xx.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "./../../utils/timer_software.h"
#include "./wifi_Manager.h"
#include "./wifi_utils.h"
#include "./../../hal/spi.h"
#include "./../LP_Mode_Manager/LP_Mode_Manager.h"
#include "./../ReaderManager/reader_manager.h"

#define ESP_RESET_PIN_NUM_U8 ((uint8_t)25U)
#define ESP_INIT_RETRIES_NUM_U8 ((uint8_t)3U)

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
																									
																									/*Set ESP to Station mode*/
																									[STATION_MODE_EN] = {"AT+CWMODE=1\r\n", (uint16_t)13, (uint8_t)2}, 
																									
																									/*Connect to WI-FI Router*/
																									[CONNECT_WI_FI_EN] = {"AT+CWJAP=\"TP-Link_4FE4\",\"00773126\"\r\n", (uint16_t)36, (uint8_t)2},
																									//[CONNECT_WI_FI_EN] = {"AT+CWJAP=\"DIGI-02349788\",\"gy3cUath\"\r\n", (uint16_t)37, (uint8_t)2},
																									
																									/*Allow multiple connections in order to put module on TCP Server mode*/
																									[ALLOW_MULTIPLE_CONNECTIONS_EN] = {"AT+CIPMUX=1\r\n", (uint16_t)13, (uint8_t)2}, 
																									
																									/*Limit only to 1 connection*/
																									[LIMIT_TO_1_CONNECTION] = {"AT+CIPSERVERMAXCONN=1\r\n", (uint16_t)23, (uint8_t)2},
																									
																									/*Open the TCP Server and listen for connections*/
																									[OPEN_TCP_SERVER] = {"AT+CIPSERVER=1,8080\r\n", (uint16_t)21, (uint8_t)2},
																									
																									/*Querry for IP address*/
																									[GET_STATUS_AND_IP] = {"AT+CIPSTA?\r\n", (uint16_t)12, (uint8_t)3}
																								};

static AT_response_st wifi_response_buffer[5];																											

void WifiManager_Perform_HW_Reset(void)
{
	IO0SET |= ((uint8_t)1U << ESP_RESET_PIN_NUM_U8);
	TIMER_SOFTWARE_Wait(500);
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
		/*Get the ready status after HW module is being init*/
		message_status = Read_Ready_Status();
		
		if(WI_FI_COMMAND_OK == message_status)
		{
			/*Process the INIT Config sequence*/
			for(init_seq_index = AT_EN; init_seq_index < END_OF_SEQUENCE; init_seq_index++)
			{
				/*Send and receive response for current command*/
				message_status = Send_ESP_Command(wifi_init_config[init_seq_index], wifi_response_buffer);
				if(WI_FI_COMMAND_NOK == message_status)
				{		
					/*Announce LP Manager about init's state*/
					LP_Set_InitFlag(FUNC_WIFI_MANAGER, false);
					
					/*Set Keep Awake flag to false for robustness*/
					LP_Set_StayAwake(FUNC_WIFI_MANAGER, false);
					
					/*If one command was wrong, do not continue*/
					break;
				}
			}
		}
	}
	
	module_state = Get_Module_Current_State();
	
	if((WI_FI_COMMAND_OK == message_status) && (true == module_state.HW_module_init) && (true == module_state.wifi_connected) && true == module_state.wifi_got_ip)
	{
		/*Announce LP Manager about init's state*/
		LP_Set_InitFlag(FUNC_WIFI_MANAGER, true);
		
		/*Set Keep Awake flag to false for robustness*/
		LP_Set_StayAwake(FUNC_WIFI_MANAGER, false);
		
		#ifdef WI_FI_DEBUG
		printf("\nWI FI MODULE INIT OK\n");
		#endif /*WI_FI_DEBUG*/
	}
	else
	{
		/*Announce LP Manager about init's state*/
		LP_Set_InitFlag(FUNC_WIFI_MANAGER, false);
		
		/*Set Keep Awake flag to false for robustness*/
		LP_Set_StayAwake(FUNC_WIFI_MANAGER, false);
		
		#ifdef WI_FI_DEBUG
		printf("\nWI FI MODULE INIT  NOT OK\n");
		#endif /*WI_FI_DEBUG*/
	}
}

static void Disable_WIFI_Module_HW(void)
{
	IO0CLR |= ((uint8_t)1U << ESP_RESET_PIN_NUM_U8);
}

void Wifi_Manager(void)
{
	typedef enum wifi_manager_state_en
	{
		WIFI_MAN_IDLE,
		WIFI_MAN_SEND_CMD,
		WIFI_MAN_PASSTHROUGH,
		WIFI_MAN_FAILURE,
		WIFI_MAN_INIT
	}wifi_manager_state_en;
	
	static wifi_manager_state_en wifi_manager_state = WIFI_MAN_IDLE;
	bool wifi_status_update;
	wifi_module_state_st module_state;
	static uint8_t init_retries = 0x00;
	
	switch(wifi_manager_state)
	{
		case WIFI_MAN_IDLE:
			if(false == LP_Get_Functionality_Init_State(FUNC_WIFI_MANAGER))
			{
				LP_Set_StayAwake(FUNC_WIFI_MANAGER, true);
				
				/*In case init is not done, retry*/
				wifi_manager_state = WIFI_MAN_INIT;
			}
			else
			{
				if(true == Reader_GET_internal_failure_status())
				{
					wifi_manager_state = WIFI_MAN_FAILURE;
				}
				else
				{
					LP_Set_StayAwake(FUNC_WIFI_MANAGER, false);
					
					wifi_status_update = Check_for_WiFi_Update();
					if(true == wifi_status_update)
					{
						module_state = Get_Module_Current_State();
						if(false == module_state.wifi_connected)
						{
							wifi_manager_state = WIFI_MAN_FAILURE;
						}
					}
					else
					{
						/*TBD: TO be checked for requests*/
					}
				}
			}
			break;
		
		case WIFI_MAN_INIT:
			LP_Set_StayAwake(FUNC_WIFI_MANAGER, true);
			if(init_retries < ESP_INIT_RETRIES_NUM_U8)
			{
				/*Perform module configuration. Will also perform module HW reset*/
				WifiManager_Init();
				
				/*Check if module was configured successfully*/
				if(true == LP_Get_Functionality_Init_State(FUNC_WIFI_MANAGER))
				{
					if(false == LP_Get_Functionality_Init_State(FUNC_RFID_READER_MANAGER) && (false == Reader_GET_internal_failure_status()))
					{
						/*Wake-Up Reader Manager in order to initialize again*/
						LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, true);
					}
					wifi_manager_state = WIFI_MAN_IDLE;
					init_retries = 0x00;
				}
				else
				{
					init_retries ++;
					wifi_manager_state = WIFI_MAN_INIT;
				}
			}
			else
			{
				/*If chip is not reinitialized after a number of retries, we'll call it Failure*/
				wifi_manager_state = WIFI_MAN_FAILURE;
				LP_Set_StayAwake(FUNC_WIFI_MANAGER, false);
				init_retries = 0x00;
			}
			break;
		
		case WIFI_MAN_SEND_CMD:
			
		
			wifi_manager_state = WIFI_MAN_IDLE;
			break;
		
		case WIFI_MAN_PASSTHROUGH:
			break;
		
		case WIFI_MAN_FAILURE:
			
			/*Clear Stay Awake flag*/
			LP_Set_StayAwake(FUNC_WIFI_MANAGER, false);
		
			/*Clear Init Flag*/
			LP_Set_InitFlag(FUNC_WIFI_MANAGER, false);
			
			/*If Reader is in Permanent Failure state, we have no reason to keep Wi Fi module on*/
			if(true == Reader_GET_internal_failure_status())
			{
				/*Module will be disabled*/
				Disable_WIFI_Module_HW();
			
				/*For robustness purpose -> will be like an infinite loop*/
				wifi_manager_state = WIFI_MAN_FAILURE;
			}
			else /*Reader is present, so check for WI_FI_CONNECTED status by performing polling of HANDSAKE line*/
			{
				/*TBD : To be seted to false after new pin is added*/
				LP_Set_StayAwake(FUNC_WIFI_MANAGER, true);
				wifi_status_update = Check_for_WiFi_Update();
				if(true == wifi_status_update)
				{
					module_state = Get_Module_Current_State();
					if(true == module_state.wifi_connected)
					{
						wifi_manager_state = WIFI_MAN_INIT;
					}
				}
			}
			break;
		
		default:
			/*WiFi Manager shall transit back to default state*/
			wifi_manager_state = WIFI_MAN_IDLE;
			break;
	}
}

bool Wifi_GET_is_Wifi_Connected_status(void)
{
	wifi_module_state_st module_state;
	module_state = Get_Module_Current_State();
	return module_state.wifi_connected;
}
