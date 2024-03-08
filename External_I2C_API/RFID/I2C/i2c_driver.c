#include <stdio.h>
#include <linux/i2c.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "i2c_driver.h"
#include <linux/i2c-dev.h>


#define DEBUG

#ifdef DEBUG
#define ERR(...) do { fprintf(stderr, "Error in (%s), at line (%d) : %s\n", __FILE__, __LINE__, __VA_ARGS__);}while(0)
#define DBG(...) do { fprintf(stderr, __VA_ARGS__);}while(0)
#else
#define ERR(...)
#define DBG(str, ...)
#endif /*DEBUG*/

#define I2C_DEVICE "/dev/i2c-1"

static int i2c_system_fd;

i2c_state_t i2c_init(void)
{
    i2c_state_t result = STATE_OK;

    i2c_system_fd = open(I2C_DEVICE, O_RDWR);
    if(0 > i2c_system_fd)
    {
        ERR(strerror(errno));
        result = STATE_NOK;
    }

    return result;
}

i2c_state_t i2c_sendMessage(const uint8_t *message, const uint8_t length)
{
    i2c_state_t result = STATE_OK;

    unsigned long functionalities = 0;
    struct i2c_msg i2c_message;
    struct i2c_rdwr_ioctl_data i2c_frame;
    uint8_t *tmp_plainMessage;

    tmp_plainMessage = (uint8_t *)malloc(length * sizeof(uint8_t));
    if(tmp_plainMessage == NULL)
    {
        ERR("could not allocate dynamic buffer");
        result = STATE_NOK;
    }
    else
    {
        memcpy(tmp_plainMessage, message, length);

        i2c_message.len = length;
        i2c_message.buf = tmp_plainMessage;
        i2c_message.addr = SLAVE_ADDR;
        i2c_message.flags = 0x00;

        if(0 == ioctl(i2c_system_fd, I2C_FUNCS, &functionalities))
        {
            if(functionalities & I2C_FUNC_I2C){
                DBG("I2C driver present.....\n");
            }
            else
            {
                DBG("There is no I2C driver present....\n");
            }
        }
        else
        {
            DBG("Could not read I2C functionalities....\n");
        }

        i2c_frame.msgs = &i2c_message;
        i2c_frame.nmsgs = 1; /*We generally send only one message at a time, not a group of messages*/

        if(0 > ioctl(i2c_system_fd, I2C_RDWR, &i2c_frame))
        {
            ERR(strerror(errno));
        }
        else
        {
            DBG("Message sent:\n");
            #ifdef DEBUG
            for(uint8_t i = 0x00; i< length; i++)
            {
                DBG("%02X ", message[i]);
            }
            DBG("\n");
            #endif
        }

        free(tmp_plainMessage);
    }
    return result;
}

i2c_state_t i2c_receiveMessage(uint8_t *message, const uint8_t expectedlength)
{
    #define I2C_TX_REQUEST_PENDING_U8 ((uint8_t)0x02U)
    i2c_state_t result;
    struct i2c_msg i2c_message;
    struct i2c_rdwr_ioctl_data i2c_frame;

    i2c_message.len = expectedlength;
    i2c_message.buf = message;
    i2c_message.addr = SLAVE_ADDR;
    i2c_message.flags = I2C_M_RD;

    i2c_frame.msgs = &i2c_message;
    i2c_frame.nmsgs = 1; /*We generally send only one message at a time, not a group of messages*/

    if(0 > ioctl(i2c_system_fd, I2C_RDWR, &i2c_frame))
    {
        ERR(strerror(errno));
        result = STATE_NOK;
    }
    else
    {
        result = STATE_OK;
        DBG("Message received:\n");
        #ifdef DEBUG
        for(uint8_t i = 0x00; i< expectedlength; i++)
        {
            DBG("%02X ", message[i]);
        }
        DBG("\n");
        #endif

        if(message[1] == I2C_TX_REQUEST_PENDING_U8)
        {
            result = STATE_PENDING;
        }
    }

    return result;
}

i2c_state_t i2c_DeInit(void)
{
    i2c_state_t result;
    int sys_call_result;

    sys_call_result = close(i2c_system_fd);
    if(sys_call_result < 0)
    {
        ERR(strerror(errno));
        result = STATE_NOK;
    }
    else
    {
        DBG("I2C module closed successfully\n");
        result = STATE_OK;
    }

    return result;
}

/*
int main(void)
{
    uint8_t tx_msg[4] = {0x1, 0x13, 0x13, 0x13};
    uint8_t rx_msg[4];
    i2c_init();
    i2c_sendMessage(tx_msg, 4);

    i2c_receiveMessage(rx_msg, 4);
    i2c_DeInit();
    return 0;
} */