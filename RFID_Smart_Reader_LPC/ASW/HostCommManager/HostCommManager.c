#include <stdint.h>
#include <stdio.h>

#include "../../hal/i2c.h"
#include "../ReaderManager/reader_manager.h"
#include "HostCommManager.h"
#include "../LP_Mode_Manager/LP_Mode_Manager.h"

static u8 data_buffer[100];
static u8 data_length;

static i2c_command_status_t HostComm_decode_requests(i2c_requests_t command)
{
	i2c_command_status_t result = I2C_STATE_PENDING;
	bool reader_request_status;
	
	switch(command)
	{
		case I2C_REQUEST_GET_TAGS_START:
			if(NO_REQUEST == Reader_GET_request_status())
			{
				Reader_SET_read_request(true);
				result = I2C_STATE_OK;
			}
			else if(READER_IN_FAILURE == Reader_GET_request_status() || (false == LP_Get_Functionality_Init_State(FUNC_RFID_READER_MANAGER)))
			{
				result = I2C_STATE_INVALID;
			}
			else
			{
				result = I2C_STATE_NOK;
			}
			break;
		
		case I2C_REQUEST_GET_TAG_INFO:
			if((NO_REQUEST == Reader_GET_request_status()) || (READER_IN_FAILURE == Reader_GET_request_status()))
			{
				result = I2C_STATE_INVALID;
			}
			else if(REQUEST_IN_PROGRESS == Reader_GET_request_status())
			{
				result = I2C_STATE_PENDING;
			}
			else
			{
				reader_request_status = Reader_GET_TagInformation(data_buffer, &data_length);
				if(true == reader_request_status)
				{
					i2c_set_data_bytes(data_buffer, data_length);
					result = I2C_STATE_OK;
				}else if(false == reader_request_status)
				{
					result = I2C_STATE_NOK;
				}
			}
			break;
		
		case I2C_REQUEST_PING:
			result = I2C_STATE_INVALID;
			break;
		
		case I2C_REQUEST_WAKE_UP:
			result = I2C_STATE_INVALID;
			break; 
		
		case I2C_REQUEST_GO_TO_SLEEP:
			result = I2C_STATE_INVALID;
			break;
		
		case I2C_REQUEST_INVALID:
			result = I2C_STATE_INVALID;
			break;
		
		case I2C_REQUEST_INIT_STATUS:
			if(false == LP_Get_System_Init_State())
			{
				result = I2C_STATE_NOK;
			}
			else
			{
				result = I2C_STATE_OK;
			}
			break;
		
		default: 
			result = I2C_STATE_INVALID;
	}
	return result;
}

void HostComm_Manager_Init(void)
{
	/*Dummy function: No special configurations right now but it is added in order to have all managers with an init function*/
	LP_Set_InitFlag(FUNC_HOST_COMM_MANAGER, true);
}

void HostComm_Manager(void)
{
	i2c_requests_t command;
	i2c_command_status_t command_status;
	i2c_data_ready_t RX_data_available_en;
	bool i2c_inProgress;
	
	RX_data_available_en = i2c_get_RX_ready_status();
	
	/*If data aquisition is ready ( we received a full frame)*/
	if(DATA_READY == RX_data_available_en)
	{
		LP_Set_StayAwake(FUNC_HOST_COMM_MANAGER, true);
		/*Compute CRC on length-2 bytes (all bytes except CRC from host, to see if they match*/
		if(false == i2c_check_CRC_after_RX_finish())
		{
			/*Mark CRC as invalid -> corrupted data*/
			i2c_set_command_status(I2C_STATE_INVALID);
			
			/*send response to host*/
			i2c_set_response_ready_status(DATA_READY); 
		}
		else
		{
			/*Get the current command, decode and process it, set status after processing*/
			command = i2c_get_command();
			command_status = HostComm_decode_requests(command);
			i2c_set_command_status(command_status);
			
			/*send response to host*/
			i2c_set_response_ready_status(DATA_READY); 
		}
		
		/*Only reset stayAwake flag if i2c comm is not in progress*/
		i2c_inProgress = i2c_get_request_stayAwake();
		if(false == i2c_inProgress)
		{
			/*This manager finished it's task for now, can sleep*/
			LP_Set_StayAwake(FUNC_HOST_COMM_MANAGER, false);
		}
		/*Open to new RX transmission*/
		i2c_set_RX_ready_status(DATA_NOT_READY);
	}
	else
	{
		i2c_inProgress = i2c_get_request_stayAwake();
		/*If we are in process of receiving I2C new request, but we have not achieved all bytes yet, still keep alive until transmission ends*/
		if(true == i2c_inProgress)
		{
			LP_Set_StayAwake(FUNC_HOST_COMM_MANAGER, true);
		}
		else
		{
			LP_Set_StayAwake(FUNC_HOST_COMM_MANAGER, false);
		}
	}
}
