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
    RFID_init();

    //st = RFID_sendRequest(RFID_PING, 0x00, &out_data);
    printf("%d\n", RFID_sendRequest(RFID_GET_TAGS_START, 0x00, &out_data));
    printf("%d\n", out_data);
    if(out_data == 0x00)
    {
        printf("%d\n", RFID_sendRequest(RFID_GET_GET_TAGS_INFO, 0x00, &out_data));
        printf("%d\n", out_data);
    }


    return 0;
}