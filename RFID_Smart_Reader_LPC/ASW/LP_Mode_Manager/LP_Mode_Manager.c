#include <LPC22xx.h>
#include <stdio.h>

#include "LP_Mode_Manager.h"

static volatile u8 stayAwakeReasonFlag = 0x00;
static u8 initDoneFlag = 0x00;

void LP_Set_StayAwake(functionality_t functionality, bool value)
{
	if(true == value)
	{
		stayAwakeReasonFlag |= (u8)((u8)1U << (u8)functionality);
	}
	else
	{
		stayAwakeReasonFlag &= (u8)(~((u8)1U << (u8)functionality));
	}
}
void LP_Set_InitFlag(functionality_t functionality, bool value)
{
	if(true == value)
	{
		initDoneFlag |= (u8)((u8)1U << (u8)functionality);
	}
	else
	{
		initDoneFlag &= (u8)(~((u8)1U << (u8)functionality));
	}
}

bool LP_Get_System_Init_State(void)
{
	u8 index;
	bool result = true;
	for(index = 0; index < (u8)FUNC_LENGTH; index ++)
	{
		if(0x00 == (initDoneFlag & ((u8)1U << index)))
		{
			result = false;
			break;
		}
	}
	return result;
}

bool LP_Get_Functionality_Init_State(functionality_t functionality)
{
	bool result = true;

	if(0x00 == (initDoneFlag & ((u8)1U << (u8)functionality)))
	{
		result = false;
	}
	return result;
}

void LP_Mode_Manager_Init(void)
{
	#define TIMER1_BIT_POS_U8 ((u8)2U)
	#define PWM_BIT_POS_U8 ((u8)5U)
	#define RTC_BIT_POS_U8 ((u8)9U)
	#define SPI1_BIT_POS_U8 ((u8)10U)
	#define ADC_BIT_POS_U8 ((u8)12U)
	#define CAN1_BIT_POS_U8 ((u8)13U)
	#define CAN2_BIT_POS_U8 ((u8)14U)
	#define CAN3_BIT_POS_U8 ((u8)15U)
	#define CAN4_BIT_POS_U8 ((u8)16U)
	
	/*Disable unused functionalities*/
	PCONP &= (u32)(~((u32)1U << TIMER1_BIT_POS_U8));
	PCONP &= (u32)(~((u32)1U << PWM_BIT_POS_U8));
	PCONP &= (u32)(~((u32)1U << RTC_BIT_POS_U8));
	PCONP &= (u32)(~((u32)1U << SPI1_BIT_POS_U8));
	PCONP &= (u32)(~((u32)1U << ADC_BIT_POS_U8));
	PCONP &= (u32)(~((u32)1U << CAN1_BIT_POS_U8));
	PCONP &= (u32)(~((u32)1U << CAN2_BIT_POS_U8));
	PCONP &= (u32)(~((u32)1U << CAN3_BIT_POS_U8));
	PCONP &= (u32)(~((u32)1U << CAN4_BIT_POS_U8));
	
	LP_Set_InitFlag(FUNC_LP_MODE_MANAGER, true);
}

static void disable_Timer0(void)
{
	PCONP &= (u32)(~((u32)1U << (u8)1U));
}

static void enable_Timer0(void)
{
	PCONP |= (u32)((u32)1U << (u8)1U);
}

void LP_Mode_Manager(void)
{
	/*Check if there is no reason to stay awake*/
	if(0 == stayAwakeReasonFlag)
	{
		/*Disable timer because of FIQ*/
		disable_Timer0();
		printf("Entered LP\n");
		/*Go to Idle State*/
		PCON = (u8)0x01;
		//while(stayAwakeReasonFlag==0);
		enable_Timer0();
		
		printf("Exit LP\n");
	}
	else
	{
		/*Do nothing*/
	}
}

