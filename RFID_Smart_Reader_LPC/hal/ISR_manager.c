
#include <stdio.h>
#include <lpc22xx.h>

#include "ISR_manager.h"
#include "../utils/timer_software.h"
#include "../ASW/WifiManager/wifi_handsake_ISR.h"
#include "../PlatformTypes.h"

#include "i2c_ISR.h"
#include "spi_ISR.h"

#define UART1_BUFFER_SIZE 1024U

static u8 uart1_buffer[UART1_BUFFER_SIZE];
tRingBufObject uart1_ringbuff_rx; /*TBD: To be added into an interface later for better encapsulation*/

/*************************************************************************************************************************************************
	Function: 		UART1_irq
	Description:	This is the ISR handler in case a new frame is being received (RX ISR). Data is transfered from HW buffer to SW buffer for further processing
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void UART1_irq(void) __irq
{	
	u8 ch;
	u8 iir;
	
	/* dummy read to ack interrupt for UART*/
	
	iir = U1IIR; 
	/*RDA interrupt*/
	if (((iir >> 1) & 7) == 2)
	{		
		ch = U1RBR;	
		RingBufWriteOne(&uart1_ringbuff_rx, ch);
	}
	/* ack interrupt for VIC */
	VICVectAddr = 0; 
}

/*************************************************************************************************************************************************
	Function: 		TIMER_irq
	Description:	This function is an ISR handler called each 1 ms. This is used by software timer 
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void TIMER_irq(void) __irq
{
	if ((T0IR & 1) != 0)
	{
		TIMER_SOFTWARE_ModX();
		T0IR |= 1;
	}
}

/*************************************************************************************************************************************************
	Function: 		InitInterrupt
	Description:	This will initialise the Interrupt module (the VIC module)
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void InitInterrupt(void)
{
	#define ISR_ENABLE_BIT ((u8)5U)
	
	#define I2C_ISR_NUM_U8 ((u8)9U)
	#define UART1_ISR_NUM_U8 ((u8)7U)
	#define TIMER0_ISR_NUM_U8 ((u8)4U)
	#define SPI0_ISR_NUM_U8 ((u8)10U)
	#define EINT2_ISR_NUM_U8 ((u8)16U)
	
	RingBufInit(&uart1_ringbuff_rx, uart1_buffer, UART1_BUFFER_SIZE);
	
	VICIntEnable |= (1 << TIMER0_ISR_NUM_U8) | (1 << UART1_ISR_NUM_U8) | (1 << I2C_ISR_NUM_U8) | (1 << SPI0_ISR_NUM_U8) | (1 << EINT2_ISR_NUM_U8);
	VICIntSelect |= 1 << TIMER0_ISR_NUM_U8; /*TIMER ISR will be FIQ*/
	
	VICVectCntl3 = UART1_ISR_NUM_U8 			 | (1 << ISR_ENABLE_BIT);
	VICVectCntl1 = EINT2_ISR_NUM_U8				 | (1 << ISR_ENABLE_BIT);
	VICVectCntl2 = SPI0_ISR_NUM_U8				 | (1 << ISR_ENABLE_BIT);
	VICVectCntl0 = I2C_ISR_NUM_U8 	       | (1 << ISR_ENABLE_BIT);
	
	VICVectAddr3 = (unsigned long int)UART1_irq;
	VICVectAddr1 = (unsigned long int)wifi_handsake_irq;
	VICVectAddr2 = (unsigned long int) SPI0_irq;
	VICVectAddr0 = (unsigned long int)I2C_irq;
}
