#include <stdio.h>
#include <linux/gpio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdlib.h>


#include "RFID/RFID_Manager.h"

int main()
{
    uint8_t out_data;
    RFID_request_status_t st;
    RFID_Tag_Information tag_info;
    bool system_init_status;

    RFID_init();

    st = RFID_Send_Ping();
    
    if(st == RFID_REQUEST_OK)
    {
        st = RFID_get_System_Init_Status(&system_init_status);
        if((st == RFID_REQUEST_OK) && (system_init_status == true))
        {
            st = RFID_get_Rooms(&tag_info);
            printf("Room Request Status = %d\n", st);
            if(st == RFID_REQUEST_OK)
            {
                printf("Room name: %s\n", tag_info.room_name);
                printf("Room description: %s\n", tag_info.room_description);
                printf("Room destination_status: %d\n", tag_info.destination_node);
            }
        }
    }


    return 0;
}