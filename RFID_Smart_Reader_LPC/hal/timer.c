#include <lpc22xx.h>
#include "timer.h"
#include "../utils/timer_software.h"

void TIMER_Init()
{
	T0MCR = 3;
	T0MR0 = 14745; // 1 ms
	T0IR = 1;
	T0TCR = 1;
}
