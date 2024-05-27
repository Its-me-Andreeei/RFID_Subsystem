#ifndef __WIFI_UTILS_H
#define __WIFI_UTILS_H

#include "../../PlatformTypes.h"

typedef enum command_frame_status_t
{
	WI_FI_COMMAND_OK,
	WI_FI_COMMAND_NOK
}command_frame_status_t;

typedef struct AT_response_st
{
	u8 response[100];
	u16 response_length;
}AT_response_st;

typedef struct AT_Command_st
{
	u8 at_command_name[50];
	u16 at_command_length;
	
	u8 number_of_responses;
	
}AT_Command_st;

typedef struct wifi_module_state_st
{
	bool HW_module_init;
	bool wifi_connected;
	bool wifi_got_ip;
	bool client_app_connected;
	bool passtrough_mode;
}wifi_module_state_st;

#define WIFI_PASSTHROUGH_BUFFER_LENGTH_U8 ((u8)100)

void wifi_utils_Init(void);
bool Get_HandsakePin_Status(void);
command_frame_status_t Read_Ready_Status(void);
command_frame_status_t Send_ESP_Command(AT_Command_st command, AT_response_st responses[]);
command_frame_status_t Wait_For_HIGH_Transition(void);
wifi_module_state_st Get_Module_Current_State(void);
void Set_Module_Current_State(wifi_module_state_st in_module_state);
bool Check_for_WiFi_Update(void);
bool Get_Wifi_Response_Ready_State(void);
void Set_Wifi_Response_Ready_State(bool value);
AT_response_st Get_Wifi_Response_Passtrough(void);
void Set_Passthrough_Mode(bool value);
void wifi_handsake_init(void);
bool Check_LowToHigh_Status(void);

#endif
