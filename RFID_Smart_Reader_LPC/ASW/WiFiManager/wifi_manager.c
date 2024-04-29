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
#include "../../PlatformTypes.h"

#define ESP_RESET_PIN_NUM_U8 ((u8)25U)
#define ESP_INIT_RETRIES_NUM_U8 ((u8)3U)

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
																									[AT_EN] = {"AT\r\n", (u16)4, (u8)2},
																									
																									/*Set ESP to Station mode*/
																									[STATION_MODE_EN] = {"AT+CWMODE=1\r\n", (u16)13, (u8)2}, 
																									
																									/*Connect to WI-FI Router*/
																									//[CONNECT_WI_FI_EN] = {"AT+CWJAP=\"TP-Link_4FE4\",\"00773126\"\r\n", (u16)36, (u8)2},
																									[CONNECT_WI_FI_EN] = {"AT+CWJAP=\"DIGI-02349788\",\"gy3cUath\"\r\n", (u16)37, (u8)2},
																									
																									/*Allow multiple connections in order to put module on TCP Server mode*/
																									[ALLOW_MULTIPLE_CONNECTIONS_EN] = {"AT+CIPMUX=1\r\n", (u16)13, (u8)2}, 
																									
																									/*Limit only to 1 connection*/
																									[LIMIT_TO_1_CONNECTION] = {"AT+CIPSERVERMAXCONN=1\r\n", (u16)23, (u8)2},
																									
																									/*Open the TCP Server and listen for connections*/
																									[OPEN_TCP_SERVER] = {"AT+CIPSERVER=1,8080\r\n", (u16)21, (u8)2},
																									
																									/*Querry for IP address*/
																									[GET_STATUS_AND_IP] = {"AT+CIPSTA?\r\n", (u16)12, (u8)3}
																								};
static AT_response_st wifi_response_buffer[4];
																								
static u8 wifi_passthrough_request[WIFI_PASSTHROUGH_BUFFER_LENGTH_U8];
static u8 wifi_passthrough_length;
																								
static AT_response_st wifi_passtrough_response;																																									

/*This flag is used to check for status updates if no current AT transaction is in progress*/																								
static bool request_new_passtrough_command = false;
																								
void WifiManager_Perform_HW_Reset(void)
{
	IO0SET |= ((u8)1U << ESP_RESET_PIN_NUM_U8);
	TIMER_SOFTWARE_Wait(500);
	IO0CLR |= ((u8)1U << ESP_RESET_PIN_NUM_U8);
	TIMER_SOFTWARE_Wait(1000);
	IO0SET |= ((u8)1U << ESP_RESET_PIN_NUM_U8);
	TIMER_SOFTWARE_Wait(1000);
}

static bool isPassthroughResponseValid(void)
{

	u16 index;
	
	if(NULL == strstr((char*)wifi_passtrough_response.response,"tag:"))
	{
		return false;
	}
	if(wifi_passtrough_response.response_length <= (u16)5U)
	{
		return false;
	}
	for(index = (u16)1U; index < wifi_passtrough_response.response_length; index++)
	{
		if((':' == wifi_passtrough_response.response[index]) && (':' == wifi_passtrough_response.response[index - (u16)1]))
		{
			return false;
		}
	}
	
	return true;
}
void WifiManager_Init(void)
{
	init_sequence_en init_seq_index;
	command_frame_status_t message_status = WI_FI_COMMAND_NOK;
	wifi_module_state_st module_state;
	
	/*perform init of wi-fi utils library*/
	wifi_utils_Init();
	
	/*Pin P0.25 will be used (as output) for HW reset of ESP32-C3 wi-fi module*/
	IO0DIR |= ((u32)1U << ESP_RESET_PIN_NUM_U8);
	
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
	IO0CLR |= ((u8)1U << ESP_RESET_PIN_NUM_U8);
}

void Wifi_Manager(void)
{
	typedef enum wifi_manager_state_en
	{
		WIFI_MAN_CHECK_PRECONDITION,
		WIFI_MAN_SEND_CMD,
		WIFI_MAN_ENTER_PASSTHROUGH_MODE,
		WIFI_MAN_FAILURE,
		WIFI_MAN_INIT,
		WIFI_MAN_WAIT_CLIENT,
		
		WIFI_MAN_WAIT_RESPONSE
	}wifi_manager_state_en;
	
	static wifi_manager_state_en wifi_manager_state = WIFI_MAN_CHECK_PRECONDITION;
	bool wifi_status_update;
	wifi_module_state_st module_state;
	static u8 init_retries = 0x00;
	u8 loop_index = 0x00;
	const AT_Command_st passthrough_mode[2] = {{"AT+CIPMODE=1\r\n", (u16)14U, (u8)2U}, {"AT+CIPSEND\r\n",(u16)12, (u8)2 }};
	command_frame_status_t esp_command_status;
	AT_response_st at_response[2];
	
	/*Any new updates on ESP state or messages in passtrough mode will be cought in local buffer*/
	if(true == Get_HandsakePin_Status())
	{
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
			/*No status update*/
		}
	}
	
	switch(wifi_manager_state)
	{
		case WIFI_MAN_CHECK_PRECONDITION:
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
					module_state = Get_Module_Current_State();
					if(false == module_state.client_app_connected)
					{
						wifi_manager_state = WIFI_MAN_WAIT_CLIENT;
					}
					else
					{
						wifi_manager_state = WIFI_MAN_ENTER_PASSTHROUGH_MODE;
					}
				}
			}
			break;
		
		case WIFI_MAN_INIT:
			LP_Set_StayAwake(FUNC_WIFI_MANAGER, true);
			Set_Passthrough_Mode(false);
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
					wifi_manager_state = WIFI_MAN_CHECK_PRECONDITION;
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
			/*If the previous response was read and a new request was made*/
			if((true == request_new_passtrough_command) && (false == Get_Wifi_Response_Ready_State()))
			{
				AT_Command_st command;
				
				memcpy(command.at_command_name, wifi_passthrough_request, wifi_passthrough_length);
				command.at_command_length = wifi_passthrough_length;
				command.number_of_responses = 1;
				
				esp_command_status = Send_ESP_Command(command, wifi_response_buffer);
				if(WI_FI_COMMAND_OK == esp_command_status)
				{
					wifi_manager_state = WIFI_MAN_WAIT_RESPONSE;
				}
				else
				{
					wifi_manager_state = WIFI_MAN_FAILURE;
				}
			}
			else
			{
				wifi_manager_state = WIFI_MAN_SEND_CMD;
			}
			break;
		
		case WIFI_MAN_WAIT_RESPONSE:
			/*If there is a response available from DB application, put in local buffer and provide through interface*/
			if(true == Get_Wifi_Response_Ready_State())
			{
				/*Get the actual buffer response from passthrough communication*/
				wifi_passtrough_response = Get_Wifi_Response_Passtrough();
				
				/*Reset this flag. Will be setted again when a new wifi request will be made from interface*/
				request_new_passtrough_command = false;
				
				wifi_manager_state = WIFI_MAN_SEND_CMD;
			}
			else
			{
				wifi_manager_state = WIFI_MAN_WAIT_RESPONSE;
			}
			break;
		
		case WIFI_MAN_WAIT_CLIENT:
			module_state = Get_Module_Current_State();
			/*If client app is present*/
			if(true == module_state.client_app_connected)
			{
				wifi_manager_state = WIFI_MAN_ENTER_PASSTHROUGH_MODE;
			}
			else
			{
				/*Check for state update and wait one for time (loop until client connects)*/
				wifi_status_update = Check_for_WiFi_Update();
				if(true == wifi_status_update)
				{
					/*Fetch the new state update. Will be check on next call of Wifi manager*/
					module_state = Get_Module_Current_State();
					wifi_manager_state = WIFI_MAN_WAIT_CLIENT;
				}
				else
				{
					/*There is no update of Wifi module (meaning no new client connected). Manager will wait for it*/
					wifi_manager_state = WIFI_MAN_WAIT_CLIENT;
				}
			}
			break;
		
		case WIFI_MAN_ENTER_PASSTHROUGH_MODE:
			for(loop_index = 0; loop_index < 2; loop_index ++)
			{
				esp_command_status = Send_ESP_Command(passthrough_mode[loop_index], at_response);
				if(WI_FI_COMMAND_NOK == esp_command_status)
				{
					wifi_manager_state = WIFI_MAN_FAILURE;
					break;
				}
			}
			
			if(WI_FI_COMMAND_OK == esp_command_status)
			{
				Set_Passthrough_Mode(true);
				wifi_manager_state = WIFI_MAN_SEND_CMD;
			}
			else
			{
				Set_Passthrough_Mode(false);
				wifi_manager_state = WIFI_MAN_FAILURE;
			}
			break;
		
		case WIFI_MAN_FAILURE:
			Set_Passthrough_Mode(false);	
		
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
				
				module_state = Get_Module_Current_State();
				if((true == module_state.wifi_connected) && (true == module_state.wifi_got_ip))
				{
					wifi_manager_state = WIFI_MAN_INIT;
				}
				
			}
			break;
		
		default:
			/*WiFi Manager shall transit back to default state*/
			wifi_manager_state = WIFI_MAN_CHECK_PRECONDITION;
			break;
	}
}

bool Wifi_GET_is_Wifi_Connected_status(void)
{
	wifi_module_state_st module_state;
	module_state = Get_Module_Current_State();
	return module_state.wifi_connected;
}

state_t Wifi_GET_passtrough_response(u8* out_buffer, u8 *out_length)
{
	state_t result;
	AT_response_st result_passthrough;
	
	if((out_buffer == NULL) || (out_length == NULL))
	{
		/*Reset this flag in order to process new requests*/
		request_new_passtrough_command = false;
		
		result = STATE_NOK;
	}
	else
	{
		result_passthrough = Get_Wifi_Response_Passtrough();
		if(true == Get_Wifi_Response_Ready_State())
		{
			/*Reset this flag in order to be open to send a new command*/
			
			/*Safety extra check in order to avoid negative lengths*/
			if((result_passthrough.response_length - (u8)4 > (u8)0) && (true == isPassthroughResponseValid()))
			{
				*out_length = result_passthrough.response_length;
				memcpy(out_buffer, result_passthrough.response + 4, result_passthrough.response_length - 4);
				
				/*Reset this flag in order to process new requests*/
				request_new_passtrough_command = false;
				Set_Wifi_Response_Ready_State(false);
				result = STATE_OK;
			}
			else
			{
				/*Reset this flag in order to process new requests*/
				request_new_passtrough_command = false;
				
				result = STATE_NOK;
			}
		}
		else
		{
			result = STATE_PENDING;
		}
	}
	return result;
}

bool Wifi_SET_Command_Request(const wifi_commands_en wifi_command, const u8 data_buffer[],const  u8 data_length)
{
	bool result;
	switch(wifi_command)
	{
		case WIFI_COMMAND_CHECK_TAG:
			/*Request already in progress, will be ignored*/
			if(true == request_new_passtrough_command)
			{
				result = false;
			}
			else
			{
				result = true;
				
				/*notify Wifi_Manager about new request*/
				request_new_passtrough_command = true;
				
				/*Copy input data to local buffers for later processing*/
				memcpy(wifi_passthrough_request, "tag:", strlen("tag:"));
				
				memcpy(wifi_passthrough_request + strlen("tag:") , data_buffer, data_length);
				wifi_passthrough_length = data_length + strlen("tag:");				
			}
			break;
		
		default:
			result = false;
			break;
	}
		
	return result;
}
