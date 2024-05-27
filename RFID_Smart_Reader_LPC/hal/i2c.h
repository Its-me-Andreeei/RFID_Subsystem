#ifndef __I2C_H
#define __I2C_H

#include "../PlatformTypes.h"

#define I2C_SLAVE_ADDR ((u8)0x08U)

typedef enum i2c_requests_t
{
	I2C_REQUEST_GET_TAGS_START 	= 0x00,
	I2C_REQUEST_PING 						 	= 0x01,
	I2C_REQUEST_GO_TO_SLEEP 			= 0x02,
	I2C_REQUEST_WAKE_UP						= 0x03,
	I2C_REQUEST_INIT_STATUS				= 0x04,
	I2C_REQUEST_GET_TAG_INFO	= 0x05,
	I2C_REQUEST_INVALID
}i2c_requests_t;

typedef enum data_ready_t
{
	DATA_NOT_READY,
	DATA_READY
}i2c_data_ready_t;

typedef enum i2c_command_status_t{
	I2C_STATE_OK			= 0x00,
	I2C_STATE_NOK			= 0x01,
	I2C_STATE_PENDING	= 0x02,
	I2C_STATE_INVALID = 0xFF
}i2c_command_status_t;

/**********************
General I2C frame format for DATA bytes:
TX:
[0] 			[1]			  [2]			[3]
COMMAND		DATA	 CRC_HI	 CRC_LO, DATA = 0x00 means no actual param is given, data = parameter

RX:
[0] 			[1]			[2]...[5]   [6].... [97]       [98]     [99]
COMMAND		STATUS	ROOM_ID	  	 DESCRIPTION     	CRC_HI	CRC_LO, 

Exception:
PING signal:
[0] 			[1]			[2]			[3]
0x01      0x01    0x01    0x01  (also the response is the same)

**********************/


void I2C_init(void);

i2c_requests_t i2c_get_command(void);
void i2c_set_command_status(i2c_command_status_t status);
void i2c_set_data_bytes(u8 buffer[], u8 len);
i2c_data_ready_t i2c_get_RX_ready_status(void);
void i2c_set_response_ready_status(i2c_data_ready_t data_status);
void i2c_set_RX_ready_status(i2c_data_ready_t data_status);
bool i2c_check_CRC_after_RX_finish(void);
#endif
