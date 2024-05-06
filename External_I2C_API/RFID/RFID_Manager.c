#include "RFID_Manager.h"
#include "crc.h"
#include "I2C/i2c_driver.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#define I2C_TX_MESSAGE_LEN_U8 ((uint8_t)4U)
#define I2C_RX_PING_RESPONSE_SIZE_U8 ((uint8_t)4U)
#define I2C_MESSAGE_LEN_U8 ((uint8_t)100U)
#define INVALID_OUT_DATA_U8 ((uint8_t)0xFFU)

#define COMMAND_BIT_POS_U8 ((uint8_t)0U)
#define DATA_BIT_POS_U8 ((uint8_t)1U)
#define CRC_TX_HI_BIT_POS_U8 (I2C_TX_MESSAGE_LEN_U8 - (uint8_t)2U)
#define CRC_TX_LO_BIT_POS_U8 (I2C_TX_MESSAGE_LEN_U8 - (uint8_t)1U)

#define CRC_RX_HI_BIT_POS_U8 (I2C_MESSAGE_LEN_U8 - (uint8_t)2U)
#define CRC_RX_LO_BIT_POS_U8 (I2C_MESSAGE_LEN_U8 - (uint8_t)1U)

typedef enum RFID_command_t
{
    RFID_GET_TAGS_START= 0x00,
    RFID_PING = 0x01,
    RFID_GET_SYS_INIT_STATUS = 0x04,
    RFID_GET_GET_TAGS_INFO = 0x05,
    RFID_INVALID /*More requests will be added when implemented*/
   ,
}RFID_command_t;

static uint8_t rx_message[I2C_MESSAGE_LEN_U8];

RFID_request_status_t RFID_init(void)
{

    i2c_init();
}

RFID_request_status_t RFID_sendRequest(const RFID_command_t command, const uint8_t in_data, uint8_t* const status)
{

    #define I2C_RX_REQUEST_SUCCESS ((uint8_t)0x00U)
    #define I2C_RX_REQUEST_FAILURE ((uint8_t)0x01U)
    #define I2C_RX_REQUEST_PENDING ((uint8_t)0x02U)
    #define I2C_RX_REQUEST_INVALID ((uint8_t)0x03U)

    RFID_request_status_t result = RFID_REQUEST_OK;
    i2c_state_t i2c_comm_state;
    uint16_t crc;
    uint16_t index;

    uint8_t tx_message[I2C_TX_MESSAGE_LEN_U8];
    
    
    *status = INVALID_OUT_DATA_U8;

    if(command >= RFID_INVALID)
    {
        
        
        result = RFID_REQUEST_NOK_INVALID_COMMAND;
    }
    else
    {
        if(RFID_PING == command)
        {
            for(index =0 ;index < I2C_TX_MESSAGE_LEN_U8; index++)
            {
                tx_message[index] = (uint8_t)0x01U;
            }
        }
        else
        {
            tx_message[COMMAND_BIT_POS_U8] = (uint8_t)command;
            tx_message[DATA_BIT_POS_U8] = in_data;

            crc = compute_crc(tx_message, I2C_TX_MESSAGE_LEN_U8 - (uint8_t)2U);

            tx_message[CRC_TX_HI_BIT_POS_U8] = (uint8_t)((crc >> (uint8_t)8U) & (uint8_t)0xFFU);
            tx_message[CRC_TX_LO_BIT_POS_U8] = (uint8_t)(crc & (uint8_t)0xFFU);
        }
        i2c_comm_state= i2c_sendMessage(tx_message, I2C_TX_MESSAGE_LEN_U8);
        if(STATE_NOK == i2c_comm_state)
        {
            result = RFID_REQUEST_NOK_TX_COMM_ERROR;
        }
        else
        {
            do{
                if(RFID_PING == command)
                {
                   i2c_comm_state = i2c_receiveMessage(rx_message, I2C_RX_PING_RESPONSE_SIZE_U8); 
                }
                else
                {
                    i2c_comm_state = i2c_receiveMessage(rx_message, I2C_MESSAGE_LEN_U8);
                }

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
                        if(RFID_PING != rx_message[COMMAND_BIT_POS_U8])
                        {
                            crc = compute_crc(rx_message, I2C_MESSAGE_LEN_U8-2);
                            if((rx_message[CRC_RX_HI_BIT_POS_U8] !=(uint8_t)((crc >> (uint8_t)8U) & (uint8_t)0xFFU)) || 
                            (rx_message[CRC_RX_LO_BIT_POS_U8] != (uint8_t)(crc & (uint8_t)0xFFU)))
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
                                    *status = rx_message[DATA_BIT_POS_U8];
                                    result = RFID_REQUEST_OK;
                                }
                            }
                        }
                        else /*If it's a ping signal*/
                        {
                            result = RFID_REQUEST_OK;
                            for(index = 0; index < I2C_RX_PING_RESPONSE_SIZE_U8; index++)
                            {
                                if((uint8_t)0x01U != rx_message[index])
                                {
                                    result = RFID_REQUEST_NOK_RX_COMM_ERROR;
                                }
                            }
                            *status = INVALID_OUT_DATA_U8;       
                        }
                    }
                }
            }while(i2c_comm_state == RFID_REQUEST_PENDING);
        }
    }

    return result;
}

RFID_request_status_t RFID_get_Rooms(RFID_Tag_Information *out_tag_info_ptr)
{
    RFID_request_status_t result = RFID_REQUEST_OK;
    uint8_t status;
    char tag_info_buffer[100]="";
    char *ptr;
    const uint8_t start_pos = DATA_BIT_POS_U8 + (uint8_t)1U;
    uint8_t index;
    /*Start the search command*/
    result = RFID_sendRequest(RFID_GET_TAGS_START, 0x00, &status);
    if((RFID_REQUEST_OK == result) && (STATE_OK == status))
    {
        /*Get the Tag info*/
        result = RFID_sendRequest(RFID_GET_GET_TAGS_INFO, 0x00, &status);
         if((RFID_REQUEST_OK == result) && (STATE_OK == status))
         {
            /*Extract only the relevant Tag info part from response message*/
            for(index = start_pos; (index < CRC_RX_HI_BIT_POS_U8) && (rx_message[index] != 0xFF) && (rx_message[index] != '\n'); index ++)
            {
                tag_info_buffer[index - start_pos] = (char)rx_message[index];
            }
            tag_info_buffer[index]='\0';

            /*---Extract the Tag Name---*/
            ptr = strtok(tag_info_buffer, ":");
            if(ptr != NULL)
            {
                strcpy(out_tag_info_ptr->room_name, ptr);
            }
            else
            {
                return RFID_INVALID_TAG_INFORMATION;
            }

            /*---Extract the Tag Description---*/
            ptr = strtok(NULL, ":");
            if(ptr != NULL)
            {
                strcpy(out_tag_info_ptr->room_description, ptr);
            }
            else
            {
                return RFID_INVALID_TAG_INFORMATION;
            }

            /*---Extract the Destination status---*/
            ptr = strtok(NULL, ":");
            if(ptr != NULL)
            {
                if(strcmp(ptr, "1")==0)
                {
                    out_tag_info_ptr->destination_node = true;
                }
                else if(strcmp(ptr, "0")==0)
                {
                    out_tag_info_ptr->destination_node = false;
                }
                else
                {
                    return RFID_INVALID_TAG_INFORMATION;
                }
            }
            else
            {
                return RFID_INVALID_TAG_INFORMATION;
            }

         }
         else if((RFID_REQUEST_OK == result) && (0x01 == status))
         {
            return RFID_REQUEST_NO_TAGS;
         }
    }

    return result;
}

RFID_request_status_t RFID_Send_Ping(void)
{
    RFID_request_status_t result;
    uint8_t status;

    result = RFID_sendRequest(RFID_PING, 0x00, &status);

    return result;
}

RFID_request_status_t RFID_get_System_Init_Status(bool *out_init_status)
{
    RFID_request_status_t result;
    uint8_t status;

     result = RFID_sendRequest(RFID_GET_SYS_INIT_STATUS, 0x00, &status);
     
     if(result == RFID_REQUEST_OK)
     {
        if(status == 0x00)
        {
            *out_init_status = true;
        }
        else
        {
            *out_init_status = false;
        }
     }
     else
     {
        *out_init_status = false;
     }

     return result;
}