#include "watchdog.h"
#include <lpc22xx.h>
#include <stdio.h>

static volatile bool WDT_request_feed = false;

bool Wdg_isFeedRequested(void)
{
	return WDT_request_feed;
}

void Wdg_Start(Wdg_Config_en config)
{
	switch(config)
	{
		case WDG_CONFIG_HP_EN:
			WDTC = (u32)0x1893FFFF; /*7 seconds*/
			//printf("WATCHDOG HP CFG\n");
			Wdg_FeedSequence();
			break;
		
		case WDG_CONFIG_LP_EN:
			WDTC = (u32)0xFFFFFFFF; /*Maximum value possible*/
			//WDTC = (u32)0x27EFBFFF; /*3 minutes*/

			//printf("WATCHDOG LP CFG\n");
			Wdg_FeedSequence();
			break;
		
		default:
			/*Do nothing*/
			break;
	}
}

void Wdg_Init(void)
{
	WDMOD = (/*(u8)((u8)1U << (u8)1U) |*/ (u8)0x01U);
}

void Wdg_Feed()
{
	WDT_request_feed = true;
}

void Wdg_FeedSequence(void)
{
	WDFEED = (u8)0xAAu;
	WDFEED = (u8)0x55u;
	WDT_request_feed = false;
}
