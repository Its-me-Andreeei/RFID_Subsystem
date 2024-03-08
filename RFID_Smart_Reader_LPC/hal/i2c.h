#ifndef __I2C_H
#define __I2C_H

#include <stdint.h>
#include "../utils/ringbuf.h"
typedef enum state_t{
	STATE_OK,
	STATE_NOK,
	STATE_PENDING
}state_t;

#define I2C_SLAVE_ADDR ((uint8_t)0x08U)

typedef enum i2c_requests_t
{
	GET_ROUTE_STATUS,
	PING = 0x01,
	GO_TO_SLEEP,
	WAKE_UP,
	INVALID
}i2c_requests_t;

/**********************
General I2C frame format for DATA bytes:
TX:
[0] 			[1]			[2]			[3]
COMMAND		DATA	 CRC_HI	 CRC_LO, DATA = 0x00 means no actual param is given, data = parameter

RX:
[0] 			[1]			[2]			[3]
COMMAND		DATA		CRC_HI	CRC_LO, DATA = returned value after COMMAND is executed

Exception:
PING signal:
[0] 			[1]			[2]			[3]
0x01      0x01    0x01    0x01  (also the response is the same)

**********************/


void i2c_init(void);

void i2c_set_command_status(state_t status);
i2c_requests_t i2c_get_command(void);
void I2C_Comm_Manager(void);

#endif
