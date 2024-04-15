#ifndef __RFID_MANAGER_H
#define __RFID_MANAGER_H

#include <stdint.h>

typedef enum RFID_command_t
{
    RFID_GET_ROUTE_START= 0x00,
    RFID_PING = 0x01,
    RFID_GET_SYS_INIT_STATUS = 0x04,
    RFID_GET_ROUTE_STATUS = 0x05,
    RFID_INVALID /*More requests will be added when implemented*/
   ,
}RFID_command_t;

typedef enum RFID_request_status_t
{
    RFID_REQUEST_OK,
    RFID_REQUEST_PENDING,
    RFID_REQUEST_NOK_INVALID_COMMAND,
    RFID_REQUEST_NOK_INVALID_CRC,
    RFID_REQUEST_NOK_TX_COMM_ERROR,
    RFID_REQUEST_NOK_RX_COMM_ERROR
}RFID_request_status_t;

#define RFID_IS_VALID_DATA(data) ((uint8_t)data != (uint8_t)0xFFU)

RFID_request_status_t RFID_init(void);
RFID_request_status_t RFID_sendRequest(const RFID_command_t command, const uint8_t in_data, uint8_t* const out_data);

#endif
