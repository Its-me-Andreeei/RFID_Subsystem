#include <lpc22xx.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "./../../utils/timer_software.h"
#include "./wifi_Manager.h"
#include "./../../hal/spi.h"

#define ESP_RESET_PIN_NUM_U8 ((uint8_t)25U)
#define ESP_HANDSAKE_GPIO_PIN_NUM_U8 ((uint8_t)15U)

typedef enum command_frame_status_t
{
	WI_FI_COMMAND_OK,
	WI_FI_COMMAND_IN_PROGRESS,
	WI_FI_COMMAND_NOK
}command_frame_status_t;

typedef struct AT_response_st
{
	uint8_t response[20];
	uint16_t response_length;
}AT_response_st;

typedef struct AT_Command_st
{
	const uint8_t at_command_name[10];
	const uint16_t at_command_length;
	
	const uint8_t number_of_responses;
	
}AT_Command_st;

typedef enum transition_type_t
{
	HIGH_TO_LOW_TO_HIGH,
	HIGH_TO_LOW,
	LOW_TO_HIGH,
}transition_type_t;

typedef enum read_write_status_t
{
	SPI_READ,
	SPI_WRITE,
	SPI_UNAVAILABLE
}read_write_status_t;

static timer_software_handler_t timer_handsake;

void WifiManager_Perform_HW_Reset(void)
{
	IO0SET |= ((uint8_t)1U << ESP_RESET_PIN_NUM_U8);
	TIMER_SOFTWARE_Wait(1000);
	IO0CLR |= ((uint8_t)1U << ESP_RESET_PIN_NUM_U8);
	TIMER_SOFTWARE_Wait(1000);
	IO0SET |= ((uint8_t)1U << ESP_RESET_PIN_NUM_U8);
	TIMER_SOFTWARE_Wait(1000);
}

static bool Get_HandsakePin_Status(void)
{
	bool result;
	uint32_t gpio_pin_val;
	gpio_pin_val = IO0PIN & ((uint32_t)1U << ESP_HANDSAKE_GPIO_PIN_NUM_U8);
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

static read_write_status_t Get_Read_Write_Status(uint16_t *length)
{
	uint8_t buffer[7] = {(uint8_t)0x02, (uint8_t)0x04, (uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00};
	read_write_status_t result;
	uint8_t handsake_gpio_value;
	
	/*Monitor value of handsake pin*/
	handsake_gpio_value = Get_HandsakePin_Status();
	/*Handsake pin must be toggled*/
	if(true == handsake_gpio_value)
	{
		
		spi0_sendReceive_message(buffer, (uint16_t)7U);
		
		
		if(buffer[3] == (uint8_t)0x02U)
		{
			*length = 0;
			result = SPI_WRITE;
		}
		else if(buffer[3] == (uint8_t)0x01U)
		{
			*length = (buffer[6] << (uint8_t)8U) | buffer[5];
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
	uint8_t buffer[3] = {0x08, 0x00, 0x00};
	spi0_sendReceive_message(buffer, 3);
}

static void send_Write_EOF(void)
{
	uint8_t buffer[3] = {0x07, 0x00, 0x00};
	spi0_sendReceive_message(buffer, 3);
}

static command_frame_status_t Read_ESP_Data(uint8_t *out_buffer, uint16_t *out_length)
{
	uint8_t local_buffer[4095];
	uint16_t index;
	read_write_status_t wr_status;
	command_frame_status_t result; 
	uint16_t length;

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

static command_frame_status_t Write_ESP_Data(const uint8_t *in_buffer, const uint16_t in_length)
{
	uint8_t local_buffer[4095];
	uint16_t index;
	read_write_status_t wr_status;
	command_frame_status_t result; 
	uint16_t dummy_length; /*No valid length will be returned by chip in case of ESP WRITE*/
			
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
	uint8_t buffer[9];
	uint16_t length;
	uint8_t index;
	
	const uint8_t response[9] = {0x0D, 0x0A, 0x72, 0x65, 0x61, 0x64, 0x79, 0x0D, 0x0A};
	
	/*This function should get the ready status after module is initialized, in order to start processing requests*/
	Read_ESP_Data(buffer, &length);
	
	if((uint16_t)9U == length)
	{
		for(index = 0; index < (uint8_t)9U; index++)
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
	
	return result;
}

static command_frame_status_t Request_ESP_Send_Permission(const uint16_t command_length)
{
	uint8_t buffer[7];
	static uint8_t sequence_number = (uint8_t)0x00;
	command_frame_status_t result = WI_FI_COMMAND_OK;
	bool handsake_pin_status;
	
	handsake_pin_status = Get_HandsakePin_Status();
	
	/*Handsake pin must be low in order to make a new transaction request*/
	if(false == handsake_pin_status)
	{		
		/*Create message structure*/
		buffer[0] = (uint8_t)0x01;
		buffer[1] = (uint8_t)0x00;
		buffer[2] = (uint8_t)0x00;
		buffer[3] = (uint8_t)0xFE;
		buffer[4] = sequence_number;
		buffer[5] = (uint8_t)(command_length & (uint8_t)0xFFU);
		buffer[6] = (uint8_t)((command_length >> (uint8_t)8) & (uint8_t)0xFFU);
		
		spi0_sendReceive_message(buffer, (uint16_t)7U);
		
		if(sequence_number < (uint8_t)0xFF)
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

command_frame_status_t Wait_for_transition(const transition_type_t transition)
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
			while(false == Get_HandsakePin_Status())
			{
				if(0 != TIMER_SOFTWARE_interrupt_pending(timer_handsake))
				{
					result = WI_FI_COMMAND_NOK;
					break;
				}
			}
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

void Send_ESP_Command(AT_Command_st command, AT_response_st responses[])
{
	uint8_t index;
	uint16_t i_dbg;
	/*TBD: To be added error management instead of cast to void*/
	(void)Wait_for_transition(HIGH_TO_LOW);
	(void)Request_ESP_Send_Permission(command.at_command_length);
	
	(void)Wait_for_transition(LOW_TO_HIGH);
	(void)Write_ESP_Data(command.at_command_name, command.at_command_length);
	
	for(index = 0; index < command.number_of_responses; index ++)
	{
		(void)Wait_for_transition(HIGH_TO_LOW_TO_HIGH);
		(void)Read_ESP_Data(responses[index].response, &responses[index].response_length);
		
		#ifdef WI_FI_DEBUG
		printf("\nESP: ");
		for(i_dbg = 0; i_dbg< responses[index].response_length; i_dbg++)
		{
			printf("%c ", responses[index].response[i_dbg]);
		}
		printf("\n");
		#endif
	}
}

void WifiManager_Init(void)
{
	bool handsake_pin_status;
	AT_Command_st command = (AT_Command_st){"AT\r\n", (uint16_t)4, (uint8_t)2};
	AT_response_st at_response[2];
	
	timer_handsake = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer_handsake, MODE_0, 5000, 1);
	TIMER_SOFTWARE_reset_timer(timer_handsake);
	
	/*Pin P0.25 will be used (as output) for HW reset of ESP32-C3 wi-fi module*/
	IO0DIR |= ((uint32_t)1U << ESP_RESET_PIN_NUM_U8);
	
	/*Pin P0.15 will be used for GPIO Handsake polling, by default is configured as input*/
	
	WifiManager_Perform_HW_Reset();
	
	handsake_pin_status = Get_HandsakePin_Status();
	while(handsake_pin_status == 0)
	{
		/*TBD: To be added timer check*/
		handsake_pin_status = Get_HandsakePin_Status();
	}
	
	#ifdef WI_FI_DEBUG
	if(WI_FI_COMMAND_OK == Read_Ready_Status())
	{
		printf("\nWI FI MODULE INIT OK\n");
	}
	else
	{
		printf("\nWI FI MODULE NOT INIT OK\n");
	}
	#else
	/*TBD: To be added error management */
	(void)Read_Ready_Status();
	#endif
	
	Send_ESP_Command(command, at_response);
	
	#ifdef WI_FI_DEBUG
	printf("---THIS IS OUTSIDE SEND---\n");
	for(uint8_t i = 0; i < command.number_of_responses; i++)
	{
		printf("ESP:\n");
		for(uint16_t ii = 0; ii < at_response[i].response_length; ii++)
		{
			printf("%c ", at_response[i].response[ii]);
		}
		printf("\n");
	}
	#endif
	
}

void Wifi_Manager(void)
{

}
