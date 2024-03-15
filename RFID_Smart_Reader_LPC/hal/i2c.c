#include "i2c.h"
#include <lpc22xx.h>
#include "i2c_ISR.h"
#include "../utils/crc.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define I2C_BUFFER_SIZE_U8 ((uint8_t)4U)

#define I2C_COMMAND_BIT_POS_U8 ((uint8_t)0U)
#define I2C_STATUS_BIT_POS_U8 ((uint8_t)1U)
#define I2C_DATA_IN_BIT_POS_U8 ((uint8_t)1U)
#define I2C_CRC_HI_BIT_POS_U8 (I2C_BUFFER_SIZE_U8 - (uint8_t)2U)
#define I2C_CRC_LO_BIT_POS_U8 (I2C_BUFFER_SIZE_U8 - (uint8_t)1U)


static uint8_t i2c_buffer_tx[I2C_BUFFER_SIZE_U8];
static uint8_t i2c_buffer_rx[I2C_BUFFER_SIZE_U8];

static volatile i2c_data_ready_t response_ready_en = DATA_NOT_READY;
static volatile i2c_data_ready_t RX_data_available_en = DATA_NOT_READY;

void i2c_init(void)
{
	#define I2C_ENABLE_BIT_U8 ((uint8_t)6U)
	#define I2C_ACK_BIT_U8 ((uint8_t)2U)
	
	/*Enable SCL and SDA alternative functions from GPIO*/
	PINSEL0 |= 0x50U;
	
	/*I2C node will operate in SLAVE mode*/
	I2ADR = (I2C_SLAVE_ADDR << 1U) | 0x01U;
	
	/*Enable I2C interface and enable ACK option*/
	I2CONSET = ((uint8_t)1U << I2C_ACK_BIT_U8) | ((uint8_t)1U << I2C_ENABLE_BIT_U8);
}

void I2C_irq(void) __irq
{
	uint8_t RX_data_byte;
	uint16_t tx_tmp_crc;
	static uint8_t TX_index = 0U;
	static uint8_t RX_index = 0U;
	static uint8_t i2c_request_pending_buffer[I2C_BUFFER_SIZE_U8];
	static uint8_t *ptr_buffer = i2c_request_pending_buffer;
	
	#define CLEAR_ISR_BIT_U8 ((uint8_t)3U)
	#define I2C_ACK_BIT_U8 ((uint8_t)2U)
	
	#define SLAVE_RX_START_ADDRESSING ((uint8_t)0x60U)
	#define SLAVE_RX_DATA_READY ((uint8_t)0x80U)
	#define SLAVE_RX_STOP_CONDITION ((uint8_t)0xA0U)
	
	#define SLAVE_TX_START_ADDRESSING ((uint8_t)0xA8U)
	#define SLAVE_TX_DATA_READY ((uint8_t)0xB8U)
	#define SLAVE_TX_DATA_NACK ((uint8_t)0xC0U)
	#define SLAVE_TX_LAST_BYTE ((uint8_t)0xC8U)
	
	#define I2C_ACK_BIT_U8 ((uint8_t)2U)
	
	switch((uint8_t)I2STAT)
	{
		case SLAVE_RX_START_ADDRESSING:
				RX_index = 0;
				response_ready_en = DATA_NOT_READY;
				I2CONSET = (1 << I2C_ACK_BIT_U8);
			break;
		 
		case SLAVE_RX_DATA_READY:
 			RX_data_byte = (uint8_t)I2DAT;
			if(RX_index < I2C_BUFFER_SIZE_U8)
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
				/*Check If we have a PING signal, then compute it's whole value. 
				No Manager should be involved as it is only used to check communication, with instant response*/
				if(i2c_buffer_rx[I2C_COMMAND_BIT_POS_U8] == I2C_REQUEST_PING)
				{
					(void)memset(i2c_request_pending_buffer, I2C_REQUEST_PING, I2C_BUFFER_SIZE_U8);
				}
				else
				{
					i2c_request_pending_buffer[I2C_COMMAND_BIT_POS_U8] = RX_data_byte;
					i2c_buffer_tx[I2C_COMMAND_BIT_POS_U8] = RX_data_byte;
					
					i2c_request_pending_buffer[I2C_STATUS_BIT_POS_U8] = (uint8_t)STATE_PENDING;
					
					tx_tmp_crc = compute_crc(i2c_buffer_tx, I2C_BUFFER_SIZE_U8-2);
					i2c_request_pending_buffer[I2C_CRC_HI_BIT_POS_U8] = (tx_tmp_crc >> 8) & 0xFF;
					i2c_request_pending_buffer[I2C_CRC_LO_BIT_POS_U8] = tx_tmp_crc & 0xFF;
				}
				
			} /*Prepare entire temporary response, separation is done in order to not spend much time only on FIRST byte*/
			else if(RX_index >= I2C_BUFFER_SIZE_U8)
			{
				RX_index = 0;
			}
			break;
			
		case SLAVE_RX_STOP_CONDITION:
			I2CONSET = ((uint8_t)1U << I2C_ACK_BIT_U8); 
			break;
			
		case SLAVE_TX_START_ADDRESSING:
			TX_index = 0;
			
			if(I2C_REQUEST_PING == i2c_buffer_rx[I2C_COMMAND_BIT_POS_U8])
			{
				RX_data_available_en = DATA_NOT_READY;
			}
			else
			{
				RX_data_available_en = DATA_READY;
			}
		
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
			I2CONSET = ((uint8_t)1U << I2C_ACK_BIT_U8);
			TX_index ++;
			break;
		
		case SLAVE_TX_DATA_READY:
			if(TX_index < I2C_BUFFER_SIZE_U8)
			{
				I2DAT = ptr_buffer[TX_index];
				
				/*If there is only 1 byte left, mark Last Byte and send*/
				if(TX_index < (I2C_BUFFER_SIZE_U8 - 1))
				{
					I2CONSET = ((uint8_t)1U << I2C_ACK_BIT_U8);
					TX_index ++;
				}
				else
				{
 					I2CONCLR = ((uint8_t)1U << I2C_ACK_BIT_U8);
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
			I2CONSET = ((uint8_t)1U << I2C_ACK_BIT_U8);
	 		break;
			
		case SLAVE_TX_LAST_BYTE:
			/*Open to another message exchange session*/
			I2CONSET = ((uint8_t)1U << I2C_ACK_BIT_U8);
			break;
		default:
			/*No actions needed*/
			break;
	}
	
	I2CONCLR = (uint8_t)((uint8_t)1U << CLEAR_ISR_BIT_U8);
	VICVectAddr = 0;
}

void i2c_set_command_status(i2c_command_status_t status)
{
	uint16_t crc;
	
	i2c_buffer_tx[I2C_STATUS_BIT_POS_U8] = status;
	
	crc = compute_crc(i2c_buffer_tx, I2C_BUFFER_SIZE_U8-2);
	
	i2c_buffer_tx[I2C_CRC_HI_BIT_POS_U8] = (crc >> 8) & 0xFF;
	i2c_buffer_tx[I2C_CRC_LO_BIT_POS_U8] = crc & 0xFF;	
}

i2c_requests_t i2c_get_command(void)
{
	i2c_requests_t command;
	if(i2c_buffer_tx[I2C_COMMAND_BIT_POS_U8] >= (uint8_t)I2C_REQUEST_INVALID)
	{
		command = I2C_REQUEST_INVALID;
	}
	else
	{
		command = (i2c_requests_t)i2c_buffer_tx[I2C_COMMAND_BIT_POS_U8];
	}
	
	return command;
}

i2c_data_ready_t i2c_get_RX_ready_status(void)
{
	return RX_data_available_en;
}

void i2c_set_response_ready_status(i2c_data_ready_t data_status)
{
	response_ready_en = data_status;
}

void i2c_set_RX_ready_status(i2c_data_ready_t data_status)
{
	RX_data_available_en = data_status;
}

bool i2c_check_CRC_after_RX_finish(void)
{
	uint16_t crc;
	uint8_t crc_hi, crc_lo; 
	bool result = true;
	
	crc = compute_crc(i2c_buffer_rx, I2C_BUFFER_SIZE_U8-2);
	crc_hi = (uint8_t) ((crc >> (uint8_t)8U) & (uint8_t)0xFFU );
	crc_lo = (uint8_t) (crc & (uint8_t)0xFFU );
	
	if((crc_hi != i2c_buffer_rx[I2C_CRC_HI_BIT_POS_U8]) || (crc_lo != i2c_buffer_rx[I2C_CRC_LO_BIT_POS_U8]))
	{
		result = false;
	}
	
	return result;
}
