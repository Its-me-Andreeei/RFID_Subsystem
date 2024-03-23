#ifndef __LP_MODE_MANAGER_H
#define __LP_MODE_MANAGER_H

#include <stdbool.h>

typedef enum functionality_t
{
	FUNC_RFID_READER_MANAGER,
	FUNC_HOST_COMM_MANAGER,
	FUNC_WIFI_MANAGER
}functionality_t;

void LP_Mode_Manager_Init(void);
void LP_Mode_Manager(void);
void LP_Set_StayAwake(functionality_t functionality, bool value);


#endif
