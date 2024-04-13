#ifndef __LP_MODE_MANAGER_H
#define __LP_MODE_MANAGER_H

#include <stdbool.h>

typedef enum functionality_t
{
	FUNC_RFID_READER_MANAGER,
	FUNC_HOST_COMM_MANAGER,
	FUNC_WIFI_MANAGER,
	FUNC_LP_MODE_MANAGER,
	
	/*This last element is used to dynamically compute number of functionalities*/
	FUNC_LENGTH
}functionality_t;

void LP_Mode_Manager_Init(void);
void LP_Mode_Manager(void);
void LP_Set_StayAwake(functionality_t functionality, bool value);
void LP_Set_InitFlag(functionality_t functionality, bool value);
bool LP_Get_System_Init_State(void);
bool LP_Get_Functionality_Init_State(functionality_t functionality);

#endif
