#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#include "../PlatformTypes.h"

typedef enum Wdg_Config_en
{
	WDG_CONFIG_LP_EN,
	WDG_CONFIG_HP_EN
}Wdg_Config_en;

void Wdg_Init(void);
void Wdg_Start(Wdg_Config_en config);
void Wdg_Feed(void);

bool Wdg_isFeedRequested(void);
void Wdg_FeedSequence(void);

#endif
