#include <stdio.h>
#include <string.h>

#include "i2c.h"
#include <lpc22xx.h>
#include "i2c_ISR.h"
#include "../utils/crc.h"

#define I2C_RX_BUFFER_SIZE_U8 ((u8)4U)
#define I2C_TX_BUFFER_SIZE_U8 ((u8)100U)
#define I2C_PING_SIZE_U8 ((u8)4U)

#define I2C_COMMAND_BIT_POS_U8 ((u8)0U)
#define I2C_STATUS_BIT_POS_U8 ((u8)1U)

#define I2C_DATA_IN_BIT_POS_U8 ((u8)1U)

#define I2C_TX_CRC_HI_BIT_POS_U8 (I2C_TX_BUFFER_SIZE_U8 - (u8)2U)
#define I2C_TX_CRC_LO_BIT_POS_U8 (I2C_TX_BUFFER_SIZE_U8 - (u8)1U)

#define I2C_RX_CRC_HI_BIT_POS_U8 (I2C_RX_BUFFER_SIZE_U8 - (u8)2U)
#define I2C_RX_CRC_LO_BIT_POS_U8 (I2C_RX_BUFFER_SIZE_U8 - (u8)1U)



static u8 i2c_buffer_tx[I2C_TX_BUFFER_SIZE_U8];
static u8 i2c_buffer_rx[I2C_RX_BUFFER_SIZE_U8];

static volatile i2c_data_ready_t response_ready_en = DATA_NOT_READY;
static volatile i2c_data_ready_t RX_data_available_en = DATA_NOT_READY;

void I2C_init(void)
{
	#define I2C_ENABLE_BIT_U8 ((u8)6U)
	#define I2C_ACK_BIT_U8 ((u8)2U)
	
	/*Enable SCL and SDA alternative functions from GPIO*/
	PINSEL0 |= 0x50U;
	
	/*I2C node will operate in SLAVE mode*/
	I2ADR = (I2C_SLAVE_ADDR << 1U) | 0x01U;
	
	/*Enable I2C interface and enable ACK option*/
	I2CONSET = ((u8)1U << I2C_ACK_BIT_U8) | ((u8)1U << I2C_ENABLE_BIT_U8);
}

void I2C_irq(void) __irq
{
	u8 RX_data_byte;
	uint16_t tx_tmp_crc;
	static u8 TX_index = 0U;
	static u8 RX_index = 0U;
	static u8 i2c_request_pending_buffer[I2C_TX_BUFFER_SIZE_U8];
	static u8 *ptr_buffer = i2c_request_pending_buffer;
	static u8 tx_length = I2C_TX_BUFFER_SIZE_U8; /*This will be either 100 either 4 (if it is a ping signal*/
	
	#define CLEAR_ISR_BIT_U8 ((u8)3U)
	#define I2C_ACK_BIT_U8 ((u8)2U)
	
	#define SLAVE_RX_START_ADDRESSING ((u8)0x60U)
	#define SLAVE_RX_DATA_READY ((u8)0x80U)
	#define SLAVE_RX_STOP_CONDITION ((u8)0xA0U)
	
	#define SLAVE_TX_START_ADDRESSING ((u8)0xA8U)
	#define SLAVE_TX_DATA_READY ((u8)0xB8U)
	#define SLAVE_TX_DATA_NACK ((u8)0xC0U)
	#define SLAVE_TX_LAST_BYTE ((u8)0xC8U)
	
	#define I2C_ACK_BIT_U8 ((u8)2U)
	
	switch((u8)I2STAT)
	{
		case SLAVE_RX_START_ADDRESSING:
				RX_index = 0;
				response_ready_en = DATA_NOT_READY;
				I2CONSET = (1 << I2C_ACK_BIT_U8);
			break;
		 
		case SLAVE_RX_DATA_READY:
 			RX_data_byte = (u8)I2DAT;
			if(RX_index < I2C_RX_BUFFER_SIZE_U8)
			{
				i2c_buffer_rx[RX_index] = RX_data_byte;
				RX_index ++;
			}
			else
			{
				RX_index = 0;
			}
			/*If we just wrote the first byte, which is the COMMAND byte, also write it in TX buffer for response*/
			if(RX_index == (u8)1U)
			{
				/*Check If we have a PING signal, then compute it's whole value. 
				No Manager should be involved as it is only used to check communication, with instant response*/
				if(i2c_buffer_rx[I2C_COMMAND_BIT_POS_U8] == I2C_REQUEST_PING)
				{
					tx_length = I2C_PING_SIZE_U8;
					(void)memset(i2c_request_pending_buffer, I2C_REQUEST_PING, I2C_PING_SIZE_U8);
				}
				else
				{
					tx_length = I2C_TX_BUFFER_SIZE_U8;
					i2c_request_pending_buffer[I2C_COMMAND_BIT_POS_U8] = RX_data_byte;
					i2c_buffer_tx[I2C_COMMAND_BIT_POS_U8] = RX_data_byte;
					
					i2c_request_pending_buffer[I2C_STATUS_BIT_POS_U8] = (u8)I2C_STATE_PENDING;
					memset(i2c_request_pending_buffer + (I2C_STATUS_BIT_POS_U8 + 1), 0xFF, I2C_TX_BUFFER_SIZE_U8 - (I2C_STATUS_BIT_POS_U8 + 1));
					memset(i2c_buffer_tx + (I2C_STATUS_BIT_POS_U8 + 1), 0xFF, I2C_TX_BUFFER_SIZE_U8 - (I2C_STATUS_BIT_POS_U8 + 1));
					tx_tmp_crc = compute_crc(i2c_request_pending_buffer, I2C_TX_BUFFER_SIZE_U8-2);
					i2c_request_pending_buffer[I2C_TX_CRC_HI_BIT_POS_U8] = (tx_tmp_crc >> 8) & 0xFF;
					i2c_request_pending_buffer[I2C_TX_CRC_HI_BIT_POS_U8] = tx_tmp_crc & 0xFF;
					ptr_buffer = i2c_request_pending_buffer;
				}
				
			} /*Prepare entire temporary response, separation is done in order to not spend much time only on FIRST byte*/
			else if(RX_index >= I2C_RX_BUFFER_SIZE_U8)
			{
				RX_index = 0;
			}
			break;
			
		case SLAVE_RX_STOP_CONDITION:
			I2CONSET = ((u8)1U << I2C_ACK_BIT_U8); 
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
			I2CONSET = ((u8)1U << I2C_ACK_BIT_U8);
			TX_index ++;
			break;
		
		case SLAVE_TX_DATA_READY:
			if(TX_index < tx_length)
			{
				I2DAT = ptr_buffer[TX_index];
				
				/*If there is only 1 byte left, mark Last Byte and send*/
				if(TX_index < (tx_length - 1))
				{
					I2CONSET = ((u8)1U << I2C_ACK_BIT_U8);
					TX_index ++;
				}
				else
				{
 					I2CONCLR = ((u8)1U << I2C_ACK_BIT_U8);
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
			I2CONSET = ((u8)1U << I2C_ACK_BIT_U8);
	 		break;
			
		case SLAVE_TX_LAST_BYTE:
			/*Open to another message exchange session*/
			I2CONSET = ((u8)1U << I2C_ACK_BIT_U8);
			break;
		default:
			/*No actions needed*/
			break;
	}
	
	/*Clear interrupt flag of peripheral*/
	I2CONCLR = (u8)((u8)1U << CLEAR_ISR_BIT_U8);
	
	/*Acknowledge the interrupt for VIC module*/
	VICVectAddr = 0;
}

void i2c_set_data_bytes(u8 buffer[], u8 len)
{
	memcpy(i2c_buffer_tx + I2C_STATUS_BIT_POS_U8 + 1, buffer, len);
}

void i2c_set_command_status(i2c_command_status_t status)
{
	uint16_t crc;
	
	i2c_buffer_tx[I2C_STATUS_BIT_POS_U8] = status;
	
	crc = compute_crc(i2c_buffer_tx, I2C_TX_BUFFER_SIZE_U8-2);
	
	i2c_buffer_tx[I2C_TX_CRC_HI_BIT_POS_U8] = (crc >> 8) & 0xFF;
	i2c_buffer_tx[I2C_TX_CRC_LO_BIT_POS_U8] = crc & 0xFF;	
}

i2c_requests_t i2c_get_command(void)
{
	i2c_requests_t command;
	if(i2c_buffer_tx[I2C_COMMAND_BIT_POS_U8] >= (u8)I2C_REQUEST_INVALID)
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
	u8 crc_hi, crc_lo; 
	bool result = true;
	
	crc = compute_crc(i2c_buffer_rx, I2C_RX_BUFFER_SIZE_U8-2);
	crc_hi = (u8) ((crc >> (u8)8U) & (u8)0xFFU );
	crc_lo = (u8) (crc & (u8)0xFFU );
	
	if((crc_hi != i2c_buffer_rx[I2C_RX_CRC_HI_BIT_POS_U8]) || (crc_lo != i2c_buffer_rx[I2C_RX_CRC_LO_BIT_POS_U8]))
	{
		result = false;
	}
	
	return result;
}
