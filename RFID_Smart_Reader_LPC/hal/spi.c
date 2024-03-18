#include <LPC22xx.h>
#include <stdint.h>

#include "./spi.h"
#include "./spi_ISR.h"

typedef enum slave_ready_t
{
	SLAVE_READY,
	SLAVE_NOT_READY
}slave_ready_t;

/*P0.24 will act as Slave select pin for SPI0 Master mode. Only for error flags (skip flag 7)*/
#define SPI0_SLAVE_SELECT_PIN_NUM_U8 ((uint8_t)24U)

/*Interrupt position within EXT registers*/
#define EINT2_POS_U8 ((uint8_t)2U)

typedef enum slave_select_state_t
{
	PIN_LOW = 0U,
	PIN_HIGH = 1U,
}slave_select_state_t;

/*TBD: Add function for each flag, for diag module*/
#define SPI_SLAVE_ABORT_BIT_POS_MASK_U8 ((uint8_t)1U << (uint8_t)3U)
#define SPI_MODE_FAULT_BIT_POS_MASK_U8 ((uint8_t)1U << (uint8_t)4U)
#define SPI_READ_OVERRUN_BIT_POS_MASK_U8 ((uint8_t)1U << (uint8_t)5U)
#define SPI_WRITE_COLLISION_BIT_POS_MASK_U8 ((uint8_t)1U << (uint8_t)6U)
#define SPI_TRANSFER_COMPLETE_BIT_POS_MASK_U8 ((uint8_t)1U << (uint8_t)7U)

static uint8_t permanent_spi0_status;
static uint8_t current_spi0_status;

static void SPI0_set_slave_select(slave_select_state_t state)
{
	if(PIN_HIGH == state)
	{
		IO0SET |= (uint32_t)((uint32_t)1U << SPI0_SLAVE_SELECT_PIN_NUM_U8);
	}
	else if(PIN_LOW == state)
	{
		IO0CLR |= (uint32_t)((uint32_t)1U << SPI0_SLAVE_SELECT_PIN_NUM_U8);
	}
}

void SPI0_init(void)
{
	#define SPI_MASTER_MODE_BIT_POS_U8 ((uint8_t)5U)
	#define SPI_INTERRUPT_ENABLE_BIT_POS_U8 ((uint8_t)7U)
	
	/*Enable external interrupt P0.15 - EIN2 for handsake pin*/
	PINSEL0 |= (uint32_t)0x80000000U;
	
	/*Set EINT2 as Edge-sensitive*/
	EXTMODE |= ((uint8_t)1U << EINT2_POS_U8);
	
	/*Set EINT2 as rising-edge sensitive*/
	EXTPOLAR |= ((uint8_t)1U << EINT2_POS_U8);
	
	/*Enable SPI alternative functions on pins : SCK, MISO, MOSI*/
	PINSEL0 |= (uint32_t)0x1500U;
	
	/*P0.24 pin will be used as SSEL, as SPI will only operate in master mode*/
	/*Will be configured as GPIO Output (we have to set "1" to that position number)*/
	IO0DIR |= (uint32_t)((uint32_t)1U << SPI0_SLAVE_SELECT_PIN_NUM_U8);
	
	/*Default value for Slave Select is HIGH. Will be pulled low when a new tranmission will begin*/
	SPI0_set_slave_select(PIN_HIGH);
	
	/*Put SPI Controller in Master mode*/
	S0SPCR = (uint8_t)1U << SPI_MASTER_MODE_BIT_POS_U8;
	
	/*Enable SPI Peripheral interrupt*/
	S0SPCR |= ((uint8_t)1U << SPI_INTERRUPT_ENABLE_BIT_POS_U8);
	
	/*Enable SPI Clock: 1.843.200 Hz => 1,843 MHz*/
	S0SPCCR = (uint8_t)8U;
}

void GPIO_Handsake_irq(void) __irq
{
	if(0 != ((uint8_t)EXTINT & ((uint8_t)1U << EINT2_POS_U8 )))
	{
		
		
		
		/*Clear the interrupt in System Control Module*/
		EXTINT |= ((uint8_t)1U << EINT2_POS_U8);
	}
	
	/*Acknowledge the interrupt for VIC module*/
	VICVectAddr = 0;
}

void SPI0_irq (void) __irq
{
	/*Read current status which triggered interrupt*/
	current_spi0_status = (uint8_t)S0SPSR;
	
	/*Update permanent flag, to detect historical failures for diag module*/
	permanent_spi0_status |= current_spi0_status;
	
	
	
	
	/*Check if transmission is completed*/
	if(0 != (S0SPSR & SPI_TRANSFER_COMPLETE_BIT_POS_MASK_U8))
	{
		/*Disable Slave Select line, as transfer is already done*/
		SPI0_set_slave_select(PIN_LOW);
	}
	
	/*Clear SPI Interrupt Flag*/
	S0SPINT = (uint8_t)1U;
	
	/*Acknowledge the interrupt for VIC module*/
	VICVectAddr = 0;
}
