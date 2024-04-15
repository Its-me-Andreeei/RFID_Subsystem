#ifndef __WIFI_UTILS_H
#define __WIFI_UTILS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum command_frame_status_t
{
	WI_FI_COMMAND_OK,
	WI_FI_COMMAND_NOK
}command_frame_status_t;

typedef struct AT_response_st
{
	uint8_t response[100];
	uint16_t response_length;
}AT_response_st;

typedef struct AT_Command_st
{
	const uint8_t at_command_name[50];
	const uint16_t at_command_length;
	
	const uint8_t number_of_responses;
	
}AT_Command_st;

typedef struct wifi_module_state_st
{
	bool HW_module_init;
	bool wifi_connected;
	bool wifi_got_ip;
}wifi_module_state_st;

void wifi_utils_Init(void);
bool Get_HandsakePin_Status(void);
command_frame_status_t Read_Ready_Status(void);
command_frame_status_t Send_ESP_Command(AT_Command_st command, AT_response_st responses[]);
command_frame_status_t Wait_For_HIGH_Transition(void);
wifi_module_state_st Get_Module_Current_State(void);
void Set_Module_Current_State(wifi_module_state_st in_module_state);
bool Check_for_WiFi_Update(void);

#endif
