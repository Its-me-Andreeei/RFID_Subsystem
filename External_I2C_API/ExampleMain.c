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
    RFID_init();
    printf("%d\n", RFID_sendRequest(RFID_GET_ROUTE_STATUS, 0x00, &out_data));

    printf("%d\n", out_data);


    return 0;
}