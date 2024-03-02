#ifndef __I2C_DRIVER_H
#define __I2C_DRIVER_H

typedef enum state_t
{
    STATE_OK,
    STATE_PENDING,
    STATE_NOK
}state_t;

#define SLAVE_ADDR ((uint8_t)0x01U)

state_t i2c_init(void);
state_t i2c_sendMessage(const uint8_t *message, const uint8_t length);
state_t i2c_receiveMessage(const uint8_t *message, const uint8_t expectedlength);

#endif
