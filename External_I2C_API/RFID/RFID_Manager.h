#ifndef __RFID_MANAGER_H
#define __RFID_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum RFID_request_status_t
{
    RFID_REQUEST_OK,
    RFID_REQUEST_PENDING,
    RFID_REQUEST_NOK_INVALID_COMMAND,
    RFID_REQUEST_NOK_INVALID_CRC,
    RFID_REQUEST_NOK_TX_COMM_ERROR,
    RFID_REQUEST_NOK_RX_COMM_ERROR,
    RFID_INVALID_TAG_INFORMATION,
    RFID_REQUEST_NO_TAGS
}RFID_request_status_t;

typedef struct RFID_Tag_Information
{
    char room_name[20];
    char room_description[90];
    bool destination_node;
}RFID_Tag_Information;

RFID_request_status_t RFID_init(void);

RFID_request_status_t RFID_Send_Ping(void);
RFID_request_status_t RFID_get_System_Init_Status(bool *out_init_status);
RFID_request_status_t RFID_get_Rooms(RFID_Tag_Information *out_tag_info_ptr);
#endif
