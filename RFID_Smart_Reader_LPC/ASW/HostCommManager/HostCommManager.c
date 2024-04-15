#include <stdint.h>

#include "../../hal/i2c.h"
#include "../ReaderManager/reader_manager.h"
#include "HostCommManager.h"
#include "../LP_Mode_Manager/LP_Mode_Manager.h"

static i2c_command_status_t HostComm_decode_requests(i2c_requests_t command)
{
	i2c_command_status_t result = STATE_PENDING;
	route_status_t route_status;
	
	switch(command)
	{
		case I2C_REQUEST_GET_ROUTE_START:
			if(NO_REQUEST == Reader_GET_request_status())
			{
				Reader_SET_read_request(true);
				result = STATE_OK;
			}
			else if(READER_IN_FAILURE == Reader_GET_request_status())
			{
				result = STATE_INVALID;
			}
			else
			{
				result = STATE_NOK;
			}
			break;
		
		case I2C_REQUEST_GET_ROUTE_STATUS:
			if((NO_REQUEST == Reader_GET_request_status()) || (READER_IN_FAILURE == Reader_GET_request_status()))
			{
				result = STATE_INVALID;
			}
			else if(REQUEST_IN_PROGRESS == Reader_GET_request_status())
			{
				result = STATE_PENDING;
			}
			else
			{
				route_status = Reader_GET_route_status();
				if(ON_THE_ROUTE == route_status)
				{
					result = STATE_OK;
				}else if(NOT_ON_ROUTE == route_status)
				{
					result = STATE_NOK;
				}
			}
			break;
		
		case I2C_REQUEST_PING:
			result = STATE_INVALID;
			break;
		
		case I2C_REQUEST_WAKE_UP:
			result = STATE_INVALID;
			break; 
		
		case I2C_REQUEST_GO_TO_SLEEP:
			result = STATE_INVALID;
			break;
		
		case I2C_REQUEST_INVALID:
			result = STATE_INVALID;
			break;
		
		case I2C_REQUEST_INIT_STATUS:
			if(false == LP_Get_System_Init_State())
			{
				result = STATE_NOK;
			}
			else
			{
				result = STATE_OK;
			}
			break;
		
		default: 
			result = STATE_INVALID;
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
	
	RX_data_available_en = i2c_get_RX_ready_status();
	
	/*If data aquisition is ready ( we received a full frame)*/
	if(DATA_READY == RX_data_available_en)
	{
		LP_Set_StayAwake(FUNC_HOST_COMM_MANAGER, true);
		/*Compute CRC on length-2 bytes (all bytes except CRC from host, to see if they match*/
		if(false == i2c_check_CRC_after_RX_finish())
		{
			/*Mark CRC as invalid -> corrupted data*/
			i2c_set_command_status(STATE_NOK);
			
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
		
		/*This manager finished it's task for now, can sleep*/
		LP_Set_StayAwake(FUNC_HOST_COMM_MANAGER, false);
		
		/*Open to new RX transmission*/
		i2c_set_RX_ready_status(DATA_NOT_READY);
	}
}
