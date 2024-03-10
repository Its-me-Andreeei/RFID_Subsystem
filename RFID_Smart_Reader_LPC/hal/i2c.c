#include "i2c.h"
#include <lpc22xx.h>
#include "i2c_ISR.h"
#include "../utils/crc.h"
#include <stdio.h>
#include "../ASW/ReaderManager/reader_manager.h"

#define I2C_BUFFER_SIZE ((uint8_t)4U)

#define I2C_TX_REQUEST_SUCCESS ((uint8_t)0x00U)
#define I2C_TX_REQUEST_FAILURE ((uint8_t)0x01U)
#define I2C_TX_REQUEST_PENDING ((uint8_t)0x02U)
#define I2C_TX_REQUEST_INVALID ((uint8_t)0x03U)

typedef enum data_ready_t
{
	DATA_NOT_READY,
	DATA_READY
}data_ready_t;

static uint8_t i2c_request_pending_buffer[I2C_BUFFER_SIZE];
static volatile uint8_t *ptr_buffer = i2c_request_pending_buffer;
static uint8_t i2c_buffer_tx[I2C_BUFFER_SIZE];
static volatile uint8_t TX_index = 0U;
static volatile data_ready_t response_ready_en = DATA_NOT_READY;

static uint8_t i2c_buffer_rx[I2C_BUFFER_SIZE];
static volatile uint8_t RX_index = 0U;
//static const uint8_t COMSTR_TX_REQUEST_PENDING[I2C_BUFFER_SIZE] = {};

static volatile data_ready_t RX_data_available_en = DATA_NOT_READY;
//static volatile bool response_ready_to_deploy = false;
//static uint8_t temporary_response[I2C_BUFFER_SIZE];

void i2c_init(void)
{
	#define I2C_ENABLE_BIT ((uint8_t)6U)
	#define I2C_ACK_BIT ((uint8_t)2U)
	
	/*Enable SCL and SDA alternative functions from GPIO*/
	PINSEL0 |= 0x50U;
	
	/*I2C node will operate in SLAVE mode*/
	I2ADR = (I2C_SLAVE_ADDR << 1) | 0x01;
	
	/*Enable I2C interface and enable ACK option*/
	I2CONSET = ((uint8_t)1U << I2C_ACK_BIT) | ((uint8_t)1U << I2C_ENABLE_BIT);
}

void I2C_irq(void) __irq
{
	uint8_t RX_data_byte;
	uint16_t tx_tmp_crc;
	
	#define CLEAR_ISR_BIT ((uint8_t)3U)
	#define I2C_ACK_BIT ((uint8_t)2U)
	
	#define SLAVE_RX_START_ADDRESSING ((uint8_t)0x60U)
	#define SLAVE_RX_DATA_READY ((uint8_t)0x80U)
	#define SLAVE_RX_STOP_CONDITION ((uint8_t)0xA0U)
	
	#define SLAVE_TX_START_ADDRESSING ((uint8_t)0xA8U)
	#define SLAVE_TX_DATA_READY ((uint8_t)0xB8U)
	#define SLAVE_TX_DATA_NACK ((uint8_t)0xC0U)
	#define SLAVE_TX_LAST_BYTE ((uint8_t)0xC8U)
	
	#define I2C_ACK_BIT ((uint8_t)2U)
	
	//printf("I2STAT=%02X\n", I2STAT);
	switch((uint8_t)I2STAT)
	{
		case SLAVE_RX_START_ADDRESSING:
				RX_index = 0;
				response_ready_en = DATA_NOT_READY;
				I2CONSET = (1 << I2C_ACK_BIT);
			break;
		 
		case SLAVE_RX_DATA_READY:
 			RX_data_byte = (uint8_t)I2DAT;
			if(RX_index < I2C_BUFFER_SIZE)
			{
				i2c_buffer_rx[RX_index] = RX_data_byte;
				RX_index ++;
			}
			else
			{
				RX_index = 0;
			}
			/*If we just wrote the first byte, which is the COMMAND byte, also write it in TX buffer for response*/
			if(RX_index == (uint8_t)1U)
			{

				i2c_request_pending_buffer[0] = RX_data_byte;
				i2c_request_pending_buffer[1] = I2C_TX_REQUEST_PENDING;
				
				tx_tmp_crc = compute_crc(i2c_buffer_tx, I2C_BUFFER_SIZE-2);
				i2c_request_pending_buffer[I2C_BUFFER_SIZE-2] = (tx_tmp_crc >> 8) & 0xFF;
				i2c_request_pending_buffer[I2C_BUFFER_SIZE-1] = tx_tmp_crc & 0xFF;
				
			} /*Prepare entire temporary response, separation is done in order to not spend much time only on FIRST byte*/
			else if(RX_index >= I2C_BUFFER_SIZE)
			{
				RX_data_available_en = DATA_READY;
				RX_index = 0;
			}
			break;
			
		case SLAVE_RX_STOP_CONDITION:
			RX_data_available_en = DATA_READY;
			I2CONSET = ((uint8_t)1U << I2C_ACK_BIT); 
			break;
			
		case SLAVE_TX_START_ADDRESSING:
			TX_index = 0;
			RX_data_available_en = DATA_READY;
 			if(DATA_READY == response_ready_en)
			{
				ptr_buffer = i2c_buffer_tx;
				response_ready_en = DATA_NOT_READY;
			}
			else if(DATA_NOT_READY == response_ready_en)
			{
				ptr_buffer = i2c_request_pending_buffer;
			}

			I2DAT = ptr_buffer[TX_index];
			I2CONSET = ((uint8_t)1U << I2C_ACK_BIT);
			TX_index ++;
			/*for(int i=0; i< 4; i++)
				printf("%02X ", i2c_buffer_tx[i]);
			printf("\n");*/
			break;
		
		case SLAVE_TX_DATA_READY:
			if(TX_index < I2C_BUFFER_SIZE)
			{
				I2DAT = ptr_buffer[TX_index];
				
				/*If there is only 1 byte left, mark Last Byte and send*/
				if(TX_index < (I2C_BUFFER_SIZE - 1))
				{
					I2CONSET = ((uint8_t)1U << I2C_ACK_BIT);
					TX_index ++;
				}
				else
				{
 					I2CONCLR = ((uint8_t)1U << I2C_ACK_BIT);
					TX_index = 0;
				}
			}
			else
			{
				TX_index = 0;
			}
			break;
			
		case SLAVE_TX_DATA_NACK:
			/*Open to another message exchange session*/
			I2CONSET = ((uint8_t)1U << I2C_ACK_BIT);
	 		break;
			
		case SLAVE_TX_LAST_BYTE:
			/*Open to another message exchange session*/
			I2CONSET = ((uint8_t)1U << I2C_ACK_BIT);
			break;
		default:
			/*No actions needed*/
			break;
	}
	
	I2CONCLR = (uint8_t)((uint8_t)1U << CLEAR_ISR_BIT);
	VICVectAddr = 0;
}

void i2c_set_command_status(state_t status)
{
	uint16_t crc;
	
	if(STATE_OK == status)
	{
		i2c_buffer_tx[1] = I2C_TX_REQUEST_SUCCESS;
	}
	else if(STATE_NOK == status)
	{
		i2c_buffer_tx[1] = I2C_TX_REQUEST_FAILURE;
	}
	else if(STATE_PENDING == status)
	{
		i2c_buffer_tx[1] = I2C_TX_REQUEST_PENDING;
	}
	else
	{
		i2c_buffer_tx[1] = I2C_TX_REQUEST_INVALID;
	}
	
	crc = compute_crc(i2c_buffer_tx, I2C_BUFFER_SIZE-2);
	i2c_buffer_tx[I2C_BUFFER_SIZE-2] = (crc >> 8) & 0xFF;
	i2c_buffer_tx[I2C_BUFFER_SIZE-1] = crc & 0xFF;	
}

i2c_requests_t i2c_get_command(void)
{
	i2c_requests_t command;
	if(i2c_buffer_tx[0] >= (uint8_t)INVALID)
	{
		command = INVALID;
	}
	else
	{
		command = (i2c_requests_t)i2c_buffer_tx[0];
	}
	
	return command;
}

state_t i2c_map_requests(i2c_requests_t command)
{
	state_t result = STATE_PENDING;
	route_status_t route_status;
	
	switch(command)
	{
		case GET_ROUTE_STATUS:
			route_status = Reader_GET_route_status();
			if(ON_THE_ROUTE == route_status)
			{
				result = STATE_OK;
			}
			else if(NOT_ON_ROUTE == route_status)
			{
				result = STATE_NOK;
			}
			else if(ROUTE_PENDING == route_status)
			{
				result = STATE_PENDING;
			}
			break;
		
		case PING:
			result = STATE_NOK;
			break;
		
		case WAKE_UP:
			result = STATE_NOK;
			break; 
		
		case GO_TO_SLEEP:
			result = STATE_NOK;
			break;
		
		case INVALID:
			result = STATE_NOK;
			break;
		
		default: 
			result = STATE_NOK;
	}
	return result;
}

void I2C_Comm_Manager(void)
{
	uint16_t crc;
	uint8_t crc_hi, crc_lo; 
	i2c_requests_t command;
	state_t command_status;

	/*If data aquisition is ready ( we received a full frame)*/
	if(DATA_READY == RX_data_available_en)
	{
		crc = compute_crc(i2c_buffer_rx, I2C_BUFFER_SIZE-2);
		crc_hi = (uint8_t) ((crc >> (uint8_t)8U) & (uint8_t)0xFFU );
		crc_lo = (uint8_t) (crc & (uint8_t)0xFFU );
		
		if((crc_hi != i2c_buffer_rx[I2C_BUFFER_SIZE-2]) || (crc_lo != i2c_buffer_rx[I2C_BUFFER_SIZE-1]))
		{
			i2c_set_command_status(STATE_NOK);
		}
		else
		{
			command = i2c_get_command();
			command_status = i2c_map_requests(command);
			i2c_set_command_status(command_status);
			
			response_ready_en = DATA_READY;
		}
		
		RX_data_available_en = DATA_NOT_READY;
	}
}

