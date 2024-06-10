#include <lpc22xx.h>
#include <stdio.h>
#include "timer.h"
#include "timer_ISR.h"
#include "watchdog.h"

#include "../utils/timer_software.h"

static volatile bool timer1_irq_triggered = false;

void timer1_irq(void) __irq
{
	if ((T1IR & 1) != 0)
	{
		timer1_irq_triggered = true;
		Wdg_FeedSequence();
		T1IR |= 1;
	}
	
	/*Acknowledge the interrupt for VIC module*/
	VICVectAddr = 0;
}

/*************************************************************************************************************************************************
	Function: 		TIMER_irq
	Description:	This function is an ISR handler called each 5 ms. This is used by software timer 
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void TIMER_irq(void) __irq
{
	if ((T0IR & 1) != 0)
	{
		TIMER_SOFTWARE_ModX();
		T0IR |= 1;
		
		if(true == Wdg_isFeedRequested())
		{
			Wdg_FeedSequence();
		}
	}
}

void TIMER_Init()
{
	T0MCR = 3;
	//T0MR0 = 14745; // 1 ms
	T0MR0 = 73725; //5ms
	T0IR = 1;
	T0TCR = 1;
	
	T1PR = 3;
	T1MR0 = 0xED4A5680; //18 minutes
	T1MCR = 5;
	T1IR = 1;
}

void TIMER1_Start(void)
{
	T1TCR = 1;
	timer1_irq_triggered = false;
}

void TIMER1_Stop(void)
{
	T1TCR = 0; //disable timer1
	T1TCR |= 2; //reset timer1
	T1TCR = 0; //release from reset timer1
	timer1_irq_triggered = false;
}

bool TIMER1_getWakeUp_Status(void)
{
	return timer1_irq_triggered;
}


