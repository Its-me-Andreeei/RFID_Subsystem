#include <lpc22xx.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "./../../utils/timer_software.h"
#include "./wifi_Manager.h"
#include "./wifi_utils.h"
#include "./wifi_handsake_ISR.h"
#include "./../../hal/spi.h"
#include "../../PlatformTypes.h"

#define ESP_HANDSAKE_GPIO_PIN_NUM_U8 ((u8)13U)

/*Interrupt position within EXT registers*/
#define EINT2_POS_U8 ((u8)2U)

#define ESP_HANDSAKE_EINT2_PIN_NUM_U8 ((u8)15U)

static timer_software_handler_t timer_handsake;

static AT_response_st wifi_passtrough_RX;
static bool wifi_passtrough_ready = false;

static volatile bool handsake_transition_irq = false;

typedef enum transition_type_t
{
	HIGH_TO_LOW_TO_HIGH,
	HIGH_TO_LOW,
	LOW_TO_HIGH,
	HIGH_ONLY
}transition_type_t;

typedef enum read_write_status_t
{
	SPI_READ,
	SPI_WRITE,
	SPI_UNAVAILABLE
}read_write_status_t;

static wifi_module_state_st module_state;

bool Get_Wifi_Response_Ready_State(void)
{
	return wifi_passtrough_ready;
}

void Set_Wifi_Response_Ready_State(bool value)
{
	wifi_passtrough_ready = value;
}

AT_response_st Get_Wifi_Response_Passtrough(void)
{
	return wifi_passtrough_RX;
}

bool Get_HandsakePin_Status(void)
{
	bool result;
	u32 gpio_pin_val;

	gpio_pin_val = IO0PIN & ((u32)1U << ESP_HANDSAKE_GPIO_PIN_NUM_U8);
	if(0 != gpio_pin_val)
	{
		result = true;
	}
	else
	{
		result = false;
	}
	
	return result;
}

static read_write_status_t Get_Read_Write_Status(u16 *length)
{
	u8 buffer[7] = {(u8)0x02, (u8)0x04, (u8)0x00, (u8)0x00, (u8)0x00, (u8)0x00, (u8)0x00};
	read_write_status_t result;
	
	/*Handsake pin must be toggled*/
	if(true == handsake_transition_irq)
	{
		/*Reset IRQ flag for next transation*/
		handsake_transition_irq = false;
		
		spi0_sendReceive_message(buffer, (u16)7U);
		
		if(buffer[3] == (u8)0x02U)
		{
			*length = 0;
			result = SPI_WRITE;
		}
		else if(buffer[3] == (u8)0x01U)
		{
			*length = (buffer[6] << (u8)8U) | buffer[5];
			result = SPI_READ;
		}
		else
		{
			*length = 0;
			result = SPI_UNAVAILABLE;
		}
	}
	else
	{
		result = SPI_UNAVAILABLE;
	}
	
	return result;
}

static void send_Read_EOF(void)
{
	u8 buffer[3] = {0x08, 0x00, 0x00};
	spi0_sendReceive_message(buffer, 3);
}

static void send_Write_EOF(void)
{
	u8 buffer[3] = {0x07, 0x00, 0x00};
	spi0_sendReceive_message(buffer, 3);
}

command_frame_status_t Read_ESP_Data(u8 *out_buffer, u16 *out_length)
{
	u8 local_buffer[1000];
	u16 index;
	read_write_status_t wr_status;
	command_frame_status_t result; 
	u16 length;

	wr_status = Get_Read_Write_Status(&length);
	if(SPI_READ == wr_status)
	{
		*out_length = length;
		local_buffer[0] = 0x04;
		local_buffer[1] = 0x00;
		local_buffer[2] = 0x00;
	
		for(index = 3; index< length + 3; index++)
		{
			local_buffer[index] = 0x00;
		}
		spi0_sendReceive_message(local_buffer, length+3);
		
		/*Copy received data in output buffer*/
		for(index = 0; index < length; index++)
		{
			out_buffer[index] = local_buffer[index+3];
		}
		send_Read_EOF();
		result = WI_FI_COMMAND_OK;
	}
	else
	{
		result = WI_FI_COMMAND_NOK;
	}
	
	return result;
}

static command_frame_status_t Write_ESP_Data(const u8 *in_buffer, const u16 in_length)
{
	u8 local_buffer[4095];
	u16 index;
	read_write_status_t wr_status;
	command_frame_status_t result; 
	u16 dummy_length; /*No valid length will be returned by chip in case of ESP WRITE*/
			
	wr_status = Get_Read_Write_Status(&dummy_length);
	if(SPI_WRITE == wr_status)
	{
		local_buffer[0] = 0x03;
		local_buffer[1] = 0x00;
		local_buffer[2] = 0x00;
	
		for(index = 0; index < in_length; index++)
		{
			local_buffer[index + 3] = in_buffer[index];
		}
		
		spi0_sendReceive_message(local_buffer, in_length+3);

		send_Write_EOF();
		
		result = WI_FI_COMMAND_OK;
	}
	else
	{
		result = WI_FI_COMMAND_NOK;
	}
	
	return result;
}

command_frame_status_t Read_Ready_Status(void)
{
	command_frame_status_t result = WI_FI_COMMAND_OK;
	u8 buffer[9];
	u16 length;
	u8 index;
	
	const u8 response[9] = {0x0D, 0x0A, 0x72, 0x65, 0x61, 0x64, 0x79, 0x0D, 0x0A};
	
	/*This function should get the ready status after module is initialized, in order to start processing requests*/
	Read_ESP_Data(buffer, &length);
	
	if((u16)9U == length)
	{
		for(index = 0; index < (u8)9U; index++)
		{
			if(response[index] != buffer[index])
			{
				result = WI_FI_COMMAND_NOK;
				break;
			}
		}
	}
	else
	{
		result = WI_FI_COMMAND_NOK;
	}
	
	if(WI_FI_COMMAND_OK == result)
	{
		module_state.HW_module_init = true;
	}
	
	return result;
}

static command_frame_status_t Request_ESP_Send_Permission(const u16 command_length)
{
	u8 buffer[7];
	static u8 sequence_number = (u8)0x00;
	command_frame_status_t result = WI_FI_COMMAND_OK;
	bool handsake_pin_status;
	
	handsake_pin_status = Get_HandsakePin_Status();
	
	/*Handsake pin must be low in order to make a new transaction request*/
	if(false == handsake_pin_status)
	{		
		/*Create message structure*/
		buffer[0] = (u8)0x01;
		buffer[1] = (u8)0x00;
		buffer[2] = (u8)0x00;
		buffer[3] = (u8)0xFE;
		buffer[4] = sequence_number;
		buffer[5] = (u8)(command_length & (u8)0xFFU);
		buffer[6] = (u8)((command_length >> (u8)8) & (u8)0xFFU);
		
		spi0_sendReceive_message(buffer, (u16)7U);
		
		if(sequence_number < (u8)0xFF)
		{
			sequence_number++;
		}
		else
		{
			sequence_number = 0x00;
		}
		
		result = WI_FI_COMMAND_OK;
	}
	else
	{
		result = WI_FI_COMMAND_NOK;
	}
	
	return result;
}

static command_frame_status_t Wait_for_transition(const transition_type_t transition)
{
	command_frame_status_t result = WI_FI_COMMAND_OK;
	
	TIMER_SOFTWARE_reset_timer(timer_handsake);
	TIMER_SOFTWARE_start_timer(timer_handsake);
	
	switch(transition)
	{
		case HIGH_TO_LOW_TO_HIGH:
			while(true == Get_HandsakePin_Status())
			{
				if(0 != TIMER_SOFTWARE_interrupt_pending(timer_handsake))
				{
					result = WI_FI_COMMAND_NOK;
					break;
				}
			}
			while(false == handsake_transition_irq)
			{
				if(0 != TIMER_SOFTWARE_interrupt_pending(timer_handsake))
				{
					result = WI_FI_COMMAND_NOK;
					break;
				}
			}
//			if((true == handsake_transition_irq) && (WI_FI_COMMAND_OK == result))
//			{
//				/*If while exit because of IRQ flag, don't forget to reset it*/
//				handsake_transition_irq = false;
//			}
			break;
			
		case HIGH_TO_LOW:
			while(true == Get_HandsakePin_Status())
			{
				if(0 != TIMER_SOFTWARE_interrupt_pending(timer_handsake))
				{
					result = WI_FI_COMMAND_NOK;
					break;
				}
			}
			break;
		
		case LOW_TO_HIGH:
			while(false == handsake_transition_irq)
			{
				if(0 != TIMER_SOFTWARE_interrupt_pending(timer_handsake))
				{
					result = WI_FI_COMMAND_NOK;
					break;
				}
			}
			
//			if((true == handsake_transition_irq) && (WI_FI_COMMAND_OK == result))
//			{
//				/*If while exit because of IRQ flag, don't forget to reset it*/
//				handsake_transition_irq = false;
//			}
			break;
			
		case HIGH_ONLY:
			while(false == Get_HandsakePin_Status())
			{
				if(0 != TIMER_SOFTWARE_interrupt_pending(timer_handsake))
				{
					result = WI_FI_COMMAND_NOK;
					break;
				}
			}
			break;
		
		default:
			result = WI_FI_COMMAND_NOK;
			break;
	}
		
		/*If the conditions happened before timeout expired*/
		if(0 == TIMER_SOFTWARE_interrupt_pending(timer_handsake))
		{
			TIMER_SOFTWARE_stop_timer(timer_handsake);
			TIMER_SOFTWARE_clear_interrupt(timer_handsake);
			TIMER_SOFTWARE_reset_timer(timer_handsake);
			
			result = WI_FI_COMMAND_OK;
		}
	
	return result;
}

bool Check_for_WiFi_Update(void)
{
	command_frame_status_t command_status;
	u8 out_buffer[4095];
	u16 command_length;
	bool result = false;
	
	/*Will check for ESP status update and then will check for new commands*/
	command_status = Read_ESP_Data(out_buffer, &command_length);
	if(WI_FI_COMMAND_OK == command_status)
	{
		Wait_for_transition(HIGH_TO_LOW);
		if(NULL != strstr((char*)out_buffer, "WIFI DISCONNECT\r\n"))
		{
			module_state.wifi_connected = false;
			result = true;
		}
		else if(NULL != strstr((char*)out_buffer, "WIFI CONNECTED\r\n"))
		{
			module_state.wifi_connected = true;
			result = true;
		}
		else if(NULL != strstr((char*)out_buffer, "0,CONNECT\r\n"))
		{
			module_state.client_app_connected = true;
		}
		else if(NULL != strstr((char*)out_buffer, "0,CLOSED\r\n"))
		{
			module_state.client_app_connected = false;
			result = true;
		}
		else
		{
			if(true == module_state.passtrough_mode)
			{
				/*announce higher level that we found passtrough response here*/
				memcpy(wifi_passtrough_RX.response, out_buffer, command_length);
				
				wifi_passtrough_RX.response_length = command_length;
				wifi_passtrough_ready = true;
			}
			result = false;
		}
	}
	else
	{
		result = false;
	}
	return result;
}

command_frame_status_t Send_ESP_Command(AT_Command_st command, AT_response_st responses[])
{
	typedef enum esp_states_en
	{
		WIFI_CONNECTED_EN,
		WIFI_DISCONNECTED_EN,
		WIFI_GOT_IP_EN,
		WIFI_CLIENT_PRESENT,
		WIFI_CLIENT_DISCONNECTED,
		
		SEQ_LENGTH
	}esp_states_en;
	
	const char ESP_States[SEQ_LENGTH][21] = {
		[WIFI_CONNECTED_EN] = {"WIFI CONNECTED\r\n"},
		[WIFI_DISCONNECTED_EN] = {"WIFI DISCONNECT\r\n"},
		[WIFI_GOT_IP_EN] = {"WIFI GOT IP\r\n"},
		[WIFI_CLIENT_PRESENT] = {"0,CONNECT\r\n"},
		[WIFI_CLIENT_DISCONNECTED] = {"0,CLOSED\r\n"}
	};
	
	const u8 number_of_ESP_states = (u8)SEQ_LENGTH;
	int8_t index;
	u8 index_esp_states;
	command_frame_status_t operation_result = WI_FI_COMMAND_NOK;

	operation_result = Wait_for_transition(HIGH_TO_LOW);
	if(WI_FI_COMMAND_OK == operation_result)
	{
		operation_result = Request_ESP_Send_Permission(command.at_command_length);
		if(WI_FI_COMMAND_OK == operation_result)
		{
			operation_result = Wait_for_transition(LOW_TO_HIGH);
			if(WI_FI_COMMAND_OK == operation_result)
			{
				operation_result = Write_ESP_Data(command.at_command_name, command.at_command_length);
				if(WI_FI_COMMAND_OK == operation_result)
				{
					for(index = 0; index < command.number_of_responses; index ++)
					{
						operation_result = Wait_for_transition(HIGH_TO_LOW_TO_HIGH);
						if(WI_FI_COMMAND_OK == operation_result)
						{
							operation_result = Read_ESP_Data(responses[index].response, &responses[index].response_length);
							if(WI_FI_COMMAND_NOK == operation_result)
							{
								/*If one command was wrong, do not continue*/
								break;
							}
							else
							{
								for(index_esp_states = 0; index_esp_states < number_of_ESP_states; index_esp_states++)
								{
									if(memcmp(responses[index].response, ESP_States[index_esp_states], responses[index].response_length) == 0)
									{
										switch(index_esp_states)
										{
											case WIFI_CONNECTED_EN:
												module_state.wifi_connected = true;
												break;
											case WIFI_DISCONNECTED_EN:
												module_state.wifi_connected = false;
												break;
											case WIFI_GOT_IP_EN:
												module_state.wifi_got_ip = true;
												break;
											case WIFI_CLIENT_PRESENT:
												module_state.client_app_connected = true;
												break;
											case WIFI_CLIENT_DISCONNECTED:
												module_state.client_app_connected = false;
												break;
											default:
												/*Do not set any flag otherwise*/
												break;
											
										}
										/*Ignore this response, consider only AT responses*/
										index --;
										/*Messages are mutual exclusives, so only one match is necessary*/
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	#ifdef WI_FI_DEBUG
	printf("--------\n");
	for(index = 0; index < command.number_of_responses; index ++)
	{
		u16 i_dbg;
		printf("\nESP: ");
		for(i_dbg = 0; i_dbg< responses[index].response_length; i_dbg++)
		{
			printf("%c ", responses[index].response[i_dbg]);
		}
		printf("\n");
	}
	printf("--------\n");
	#endif
	
	return operation_result;
}

void wifi_utils_Init(void)
{
	timer_handsake = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer_handsake, MODE_0, 6000, 1);
	TIMER_SOFTWARE_reset_timer(timer_handsake);
	wifi_handsake_init();
}

command_frame_status_t Wait_For_HIGH_Transition(void)
{
	command_frame_status_t status;
	
	status = Wait_for_transition(HIGH_ONLY);
	
	return status;
}

wifi_module_state_st Get_Module_Current_State(void)
{
	return module_state;
}

void Set_Module_Current_State(wifi_module_state_st in_module_state)
{
	module_state = in_module_state;
}

void Set_Passthrough_Mode(bool value)
{
	module_state.passtrough_mode = value;
}

void wifi_handsake_init(void)
{
	/*P0.15 will be configured as EINT2*/
	PINSEL0 |= (u32)((u32)1U << (u8)31);
	
	/*Set EINT2 as edge-sensitive*/
	EXTMODE |= (u8)((u8)1U << EINT2_POS_U8);
	
	/*Set as rising edge sensitive*/
	EXTPOLAR |= (u8)((u8)1U << EINT2_POS_U8);
}

void wifi_handsake_irq(void) __irq
{
	if((u8)0 != ((u8)EXTINT & (u8)((u8)1U << EINT2_POS_U8)))
	{
		/*Mark the fact a rising edge transition occured*/
		handsake_transition_irq = true;
		
		/*Clear interrupt flag*/
		EXTINT |= (u8)((u8)1U << EINT2_POS_U8);
	}
	
	/*Acknowledge the interrupt for VIC module*/
	VICVectAddr = 0;
}

bool Check_LowToHigh_Status(void)
{
	return handsake_transition_irq;
}

