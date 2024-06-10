#ifndef __TIMER_H
#define __TIMER_H

#include "../PlatformTypes.h"

void TIMER_Init(void);

void TIMER1_Start(void);
void TIMER1_Stop(void);
bool TIMER1_getWakeUp_Status(void);

#endif
