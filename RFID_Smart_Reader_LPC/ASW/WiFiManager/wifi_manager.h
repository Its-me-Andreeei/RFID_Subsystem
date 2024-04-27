#ifndef __WIFI_MANAGER_H
#define __WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "../../PlatformTypes.h"

typedef enum wifi_commands_en
{
	WIFI_COMMAND_CHECK_TAG,
	WIFI_COMMAND_UNAVAILABLE
}wifi_commands_en;

void Wifi_Manager(void);
void WifiManager_Init(void);
bool Wifi_GET_is_Wifi_Connected_status(void);
state_t Wifi_GET_passtrough_response(uint8_t* out_buffer, uint8_t *out_length);
bool Wifi_SET_Command_Request(const wifi_commands_en wifi_command, const uint8_t data_buffer[], const uint8_t data_length);

#endif
