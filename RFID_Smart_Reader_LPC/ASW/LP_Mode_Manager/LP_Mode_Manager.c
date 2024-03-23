#include <LPC22xx.h>
#include <stdio.h>
#include <stdint.h>

#include "LP_Mode_Manager.h"

static uint8_t stayAwakeReasonFlag = 0x00;

void LP_Set_StayAwake(functionality_t functionality, bool value)
{
	if(true == value)
	{
		stayAwakeReasonFlag |= (uint8_t)((uint8_t)1U << (uint8_t)functionality);
	}
	else
	{
		stayAwakeReasonFlag &= (uint8_t)(~((uint8_t)1U << (uint8_t)functionality));
	}
}

void LP_Mode_Manager_Init(void)
{
	#define TIMER1_BIT_POS_U8 ((uint8_t)2U)
	#define PWM_BIT_POS_U8 ((uint8_t)5U)
	#define RTC_BIT_POS_U8 ((uint8_t)9U)
	#define SPI1_BIT_POS_U8 ((uint8_t)10U)
	#define ADC_BIT_POS_U8 ((uint8_t)12U)
	#define CAN1_BIT_POS_U8 ((uint8_t)13U)
	#define CAN2_BIT_POS_U8 ((uint8_t)14U)
	#define CAN3_BIT_POS_U8 ((uint8_t)15U)
	#define CAN4_BIT_POS_U8 ((uint8_t)16U)
	
	/*Disable unused functionalities*/
	PCONP &= (uint32_t)(~((uint32_t)1U << TIMER1_BIT_POS_U8));
	PCONP &= (uint32_t)(~((uint32_t)1U << PWM_BIT_POS_U8));
	PCONP &= (uint32_t)(~((uint32_t)1U << RTC_BIT_POS_U8));
	PCONP &= (uint32_t)(~((uint32_t)1U << SPI1_BIT_POS_U8));
	PCONP &= (uint32_t)(~((uint32_t)1U << ADC_BIT_POS_U8));
	PCONP &= (uint32_t)(~((uint32_t)1U << CAN1_BIT_POS_U8));
	PCONP &= (uint32_t)(~((uint32_t)1U << CAN2_BIT_POS_U8));
	PCONP &= (uint32_t)(~((uint32_t)1U << CAN3_BIT_POS_U8));
	PCONP &= (uint32_t)(~((uint32_t)1U << CAN4_BIT_POS_U8));
}

static void disable_Timer0(void)
{
	PCONP &= (uint32_t)(~((uint32_t)1U << (uint8_t)1U));
}

static void enable_Timer0(void)
{
	PCONP |= (uint32_t)((uint32_t)1U << (uint8_t)1U);
}

void LP_Mode_Manager(void)
{
	/*Check if there is no reason to stay awake*/
	if(0 == stayAwakeReasonFlag)
	{
		/*Disable timer because of FIQ*/
		disable_Timer0();
		
		/*Go to Idle State*/
		PCON = (uint8_t)0x01;
		enable_Timer0();
	}
	else
	{
		/*Do nothing*/
	}
}

