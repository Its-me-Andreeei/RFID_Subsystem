#include "i2c.h"
#include <lpc22xx.h>
#include "../utils/ringbuf.h"
#include "i2c_ISR.h"
#include "../utils/crc.h"

#define I2C_BUFFER_SIZE ((uint8_t)5U)

#define I2C_TX_REQUEST_SUCCESS ((uint8_t)0x00U)
#define I2C_TX_REQUEST_FAILURE ((uint8_t)0x01U)
#define I2C_TX_REQUEST_PENDING ((uint8_t)0x02U)
#define I2C_TX_REQUEST_INVALID ((uint8_t)0x03U)

static uint8_t i2c_buffer_tx[I2C_BUFFER_SIZE];
tRingBufObject i2c_ringbuff_tx;

static uint8_t i2c_buffer_rx[I2C_BUFFER_SIZE];
tRingBufObject i2c_ringbuff_rx;

static uint8_t temporary_response[I2C_BUFFER_SIZE];
typedef enum available_status_t
{
	AVAILABLE,
	UNAVAILABLE
}available_status_t;

static volatile available_status_t temporary_buffer_availability = UNAVAILABLE;
static volatile state_t request_status = STATE_PENDING;

void i2c_init(void)
{
	#define I2C_ENABLE_BIT ((uint8_t)6U)
	#define I2C_ACK_BIT ((uint8_t)2U)
	
	RingBufInit(&i2c_ringbuff_rx, i2c_buffer_rx, I2C_BUFFER_SIZE);
	RingBufInit(&i2c_ringbuff_tx, i2c_buffer_tx, I2C_BUFFER_SIZE);
	
	/*Enable SCL and SDA alternative functions from GPIO*/
	PINSEL0 |= 0x50U;
	
	/*I2C node will operate in SLAVE mode*/
	I2ADR = (I2C_SLAVE_ADDR << 1) | 0x01;
	
	/*Enable I2C interface and enable ACK option*/
	I2CONSET |= ((uint8_t)1U << I2C_ACK_BIT) | ((uint8_t)1U << I2C_ENABLE_BIT);
}

void I2C_irq(void) __irq
{
	uint8_t RX_data_byte;
	uint16_t tx_tmp_crc;
	
	#define CLEAR_ISR_BIT ((uint8_t)3U)
	#define I2C_ACK_BIT ((uint8_t)2U)
	
	#define SLAVE_RX_START_ADDRESSING ((uint8_t)0x60U)
	#define SLAVE_RX_DATA_READY ((uint8_t)0x80U)
	
	#define SLAVE_TX_START_ADDRESSING ((uint8_t)0xA8U)
	#define SLAVE_TX_DATA_READY ((uint8_t)0xB8U)
	#define SLAVE_TX_LAST_BYTE ((uint8_t)0xC8U)
	
	#define I2C_ACK_BIT ((uint8_t)2U)
	
	switch((uint8_t)I2STAT)
	{
		case SLAVE_RX_START_ADDRESSING:
				temporary_buffer_availability = UNAVAILABLE;
			break;
		 
		case SLAVE_RX_DATA_READY:
 			RX_data_byte = (uint8_t)I2DAT;
			RingBufWriteOne(&i2c_ringbuff_rx, RX_data_byte);
		
			/*If we just wrote the first byte, which is the COMMAND byte, also write it in TX buffer for response*/
			if((uint8_t)RingBufUsed(&i2c_ringbuff_rx) == (uint8_t)1U)
			{
				temporary_response[0] = RX_data_byte;
				temporary_response[1] = I2C_TX_REQUEST_PENDING;
			} /*Prepare entire temporary response, separation is done in order to not spend much time only on FIRST byte*/
			else if((uint8_t)RingBufUsed(&i2c_ringbuff_rx) == I2C_FRAME_LENGTH)
			{
				tx_tmp_crc = compute_crc(temporary_response, I2C_BUFFER_SIZE-2);
				temporary_response[I2C_BUFFER_SIZE-2] = (tx_tmp_crc >> 8) & 0xFF;
				temporary_response[I2C_BUFFER_SIZE-1] = tx_tmp_crc & 0xFF;
				temporary_buffer_availability = AVAILABLE;
				
				RingBufWrite(&i2c_ringbuff_tx, temporary_response, I2C_BUFFER_SIZE);
			}
			break;
			
		case SLAVE_TX_START_ADDRESSING:
			if(RingBufEmpty(&i2c_ringbuff_tx) != 0)
			{
				I2DAT = (uint8_t)RingBufReadOne(&i2c_ringbuff_tx);
				/*If there is only 1 byte left, mark Last Byte and send*/
				if(RingBufUsed(&i2c_ringbuff_tx) == 1)
				{
					I2CONCLR = (1U << I2C_ACK_BIT);
				}
			}
			break;
		
		case SLAVE_TX_DATA_READY:
			if(RingBufEmpty(&i2c_ringbuff_tx) != 0)
			{
				/*If there is only 1 byte left, mark Last Byte and send*/
				if(RingBufUsed(&i2c_ringbuff_tx) == 1)
				{
					I2CONCLR = (1U << I2C_ACK_BIT);
				}
				I2DAT = RingBufReadOne(&i2c_ringbuff_tx);
			}
			else
			{
				/*If there is not available data to send*/
				I2CONCLR = ((uint8_t)1U << I2C_ACK_BIT);
				I2DAT = ((uint8_t)0xFFU);
			}
			break;
			
		case SLAVE_TX_LAST_BYTE:
			/*Open to another message exchange session*/
			I2CONSET |= ((uint8_t)1U << I2C_ACK_BIT);
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
		temporary_response[1] = I2C_TX_REQUEST_SUCCESS;
	}
	else if(STATE_NOK == status)
	{
		temporary_response[1] = I2C_TX_REQUEST_FAILURE;
	}
	else if(STATE_PENDING == status)
	{
		temporary_response[1] = I2C_TX_REQUEST_PENDING;
	}
	else
	{
		temporary_response[1] = I2C_TX_REQUEST_INVALID;
	}
	
	crc = compute_crc(temporary_response, I2C_BUFFER_SIZE-2);
	temporary_response[I2C_BUFFER_SIZE-2] = (crc >> 8) & 0xFF;
	temporary_response[I2C_BUFFER_SIZE-1] = crc & 0xFF;	
}

i2c_requests_t i2c_get_command(void)
{
	i2c_requests_t command;
	if(temporary_response[0] >= (uint8_t)INVALID)
	{
		command = INVALID;
	}
	else
	{
		command = (i2c_requests_t)temporary_response[0];
	}
	
	return command;
}



void I2C_Comm_Manager(void)
{
	
}

