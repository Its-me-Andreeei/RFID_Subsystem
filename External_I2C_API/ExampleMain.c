#include <stdio.h>
#include <linux/gpio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "RFID/RFID_Manager.h"

void delay(int milli_seconds)
{
    // Storing start time
    clock_t start_time = clock();
 
    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds)
        ;
}

int main()
{
    uint8_t out_data;
    RFID_request_status_t st;
    RFID_Tag_Information tag_info;
    bool op_status;
    bool system_init_status;

    RFID_init();

    //st = RFID_Send_Ping();
    
    if(st == RFID_REQUEST_OK)
    {
        st = RFID_start_get_System_Init_Status(&system_init_status);
        printf("%d\n", st);
        while(st == RFID_REQUEST_PENDING)
        {
            delay(500);
            st = RFID_status_get_System_Init_Status(&system_init_status);
        }

        if((st == RFID_REQUEST_OK) && (system_init_status == true))
        {
            st = RFID_start_Room_Search(&op_status);

            while(st == RFID_REQUEST_PENDING)
            {
                delay(500);
                st = RFID_status_Room_Search(&op_status);
            }

            if(st == RFID_REQUEST_OK && op_status == true)
            {
                printf("Room Request Status = %d\n", st);
                st=RFID_start_get_Rooms(&op_status);

                while(st == RFID_REQUEST_PENDING)
                {
                    delay(1000);
                    st = RFID_status_get_Rooms(&tag_info);
                }
                printf("Room Request Status = %d\n", st);

                if(st == RFID_REQUEST_OK)
                {
                    printf("Room name: %s\n", tag_info.room_name);
                    printf("Room description: %s\n", tag_info.room_description);
                    printf("Room destination_status: %d\n", tag_info.destination_node);
                }
            }
        }
    }

    return 0;
}