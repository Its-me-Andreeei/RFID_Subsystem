#include "RFID_Manager.h"
#include "crc.h"
#include "I2C/i2c_driver.h"
#include <stdbool.h>

#ifndef NULL
#define NULL ((void*)0)
#endif
#define I2C_MESSAGE_LEN_U8 ((uint8_t)4U)
#define INVALID_OUT_DATA_U8 ((uint8_t)0xFFU)

#define COMMAND_BIT_POS_U8 ((uint8_t)0U)
#define DATA_BIT_POS_U8 ((uint8_t)1U)
#define CRC_HI_BIT_POS_U8 ((uint8_t)2U)
#define CRC_LO_BIT_POS_U8 ((uint8_t)3U)

RFID_request_status_t RFID_init(void)
{

    i2c_init();
}

RFID_request_status_t RFID_sendRequest(const RFID_command_t command, const uint8_t in_data, uint8_t* const out_data)
{

    #define I2C_RX_REQUEST_SUCCESS ((uint8_t)0x00U)
    #define I2C_RX_REQUEST_FAILURE ((uint8_t)0x01U)
    #define I2C_RX_REQUEST_PENDING ((uint8_t)0x02U)
    #define I2C_RX_REQUEST_INVALID ((uint8_t)0x03U)

    RFID_request_status_t result = RFID_REQUEST_OK;
    i2c_state_t i2c_comm_state;
    uint16_t crc;

    uint8_t tx_message[I2C_MESSAGE_LEN_U8];
    uint8_t rx_message[I2C_MESSAGE_LEN_U8];
    
    *out_data = INVALID_OUT_DATA_U8;

    if(command >= RFID_INVALID)
    {
        
        result = RFID_REQUEST_NOK_INVALID_COMMAND;
    }
    else
    {
        tx_message[COMMAND_BIT_POS_U8] = (uint8_t)command;
        tx_message[DATA_BIT_POS_U8] = in_data;

        crc = compute_crc(tx_message, I2C_MESSAGE_LEN_U8 - (uint8_t)2U);

        tx_message[CRC_HI_BIT_POS_U8] = (uint8_t)((crc >> (uint8_t)8U) & (uint8_t)0xFFU);
        tx_message[CRC_LO_BIT_POS_U8] = (uint8_t)(crc & (uint8_t)0xFFU);

        i2c_comm_state= i2c_sendMessage(tx_message, I2C_MESSAGE_LEN_U8);
        if(STATE_NOK == i2c_comm_state)
        {
            result = RFID_REQUEST_NOK_TX_COMM_ERROR;
        }
        else
        {
            do{
                i2c_comm_state = i2c_receiveMessage(rx_message, I2C_MESSAGE_LEN_U8);
                if(STATE_NOK == i2c_comm_state)
                {
                    result = RFID_REQUEST_NOK_RX_COMM_ERROR;
                }
                else
                {
                    if(rx_message[COMMAND_BIT_POS_U8] != tx_message[COMMAND_BIT_POS_U8])
                    {
                        result = RFID_REQUEST_NOK_RX_COMM_ERROR;
                    }
                    else
                    {
                        crc = compute_crc(rx_message, I2C_MESSAGE_LEN_U8-2);
                        if((rx_message[CRC_HI_BIT_POS_U8] !=(uint8_t)((crc >> (uint8_t)8U) & (uint8_t)0xFFU)) || 
                        (rx_message[CRC_LO_BIT_POS_U8] != (uint8_t)(crc & (uint8_t)0xFFU)))
                        {
                            result = RFID_REQUEST_NOK_INVALID_CRC;
                        }
                        else
                        {
                            if(rx_message[DATA_BIT_POS_U8] == I2C_RX_REQUEST_PENDING)
                            {
                                result = RFID_REQUEST_PENDING;
                            }
                            else
                            {
                                *out_data = rx_message[DATA_BIT_POS_U8];
                                result = RFID_REQUEST_OK;
                            }
                        }
                    }
                }
            }while(i2c_comm_state == RFID_REQUEST_PENDING);
        }
    }

    return result;
}
