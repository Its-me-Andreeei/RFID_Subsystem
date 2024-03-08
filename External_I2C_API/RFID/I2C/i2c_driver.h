#ifndef __I2C_DRIVER_H
#define __I2C_DRIVER_H

typedef enum i2c_state_t
{
    STATE_OK,
    STATE_PENDING,
    STATE_NOK
}i2c_state_t;

#define SLAVE_ADDR ((uint8_t)0x08U)

i2c_state_t i2c_init(void);
i2c_state_t i2c_sendMessage(const uint8_t *message, const uint8_t length);
i2c_state_t i2c_receiveMessage(uint8_t *message, const uint8_t expectedlength);
i2c_state_t i2c_DeInit(void);

#endif
