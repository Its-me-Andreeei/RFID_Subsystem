#include <LPC22xx.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "./spi.h"
#include "./spi_ISR.h"
#include "./../utils/timer_software.h"


/*P0.24 will act as Slave select pin for SPI0 Master mode. Only for error flags (skip flag 7)*/
#define SPI0_SLAVE_SELECT_PIN_NUM_U8 ((uint8_t)24U)

/*Interrupt position within EXT registers*/
#define EINT2_POS_U8 ((uint8_t)2U)

/*TBD: Add function for each flag, for diag module*/
#define SPI_SLAVE_ABORT_BIT_POS_MASK_U8 ((uint8_t)1U << (uint8_t)3U)
#define SPI_MODE_FAULT_BIT_POS_MASK_U8 ((uint8_t)1U << (uint8_t)4U)
#define SPI_READ_OVERRUN_BIT_POS_MASK_U8 ((uint8_t)1U << (uint8_t)5U)
#define SPI_WRITE_COLLISION_BIT_POS_MASK_U8 ((uint8_t)1U << (uint8_t)6U)
#define SPI_TRANSFER_COMPLETE_BIT_POS_MASK_U8 ((uint8_t)1U << (uint8_t)7U)

#define SPI_BUFFER_NUM_HEADER_BYTES_U8 ((uint8_t)3U)
#define SPI_BUFFER_NUM_DATA_BYTES 	 ((uint8_t)4092U)

typedef enum slave_select_state_t
{
	PIN_LOW = 0U,
	PIN_HIGH = 1U,
}slave_select_state_t;

typedef enum message_status_t
{
	MESSAGE_IDLE,
	MESSAGE_TX_IN_PROGRESS,
	MESSAGE_RX_COMPLETED
}message_status_t;

typedef struct spi_response_t
{
	uint8_t response[SPI_BUFFER_NUM_DATA_BYTES];
	uint16_t response_length;
}spi_response_t;

/*Buffer will be filled with TX data which progressively will be filled with RX data*/
static uint8_t spi0_TX_RX_buffer[SPI_BUFFER_NUM_HEADER_BYTES_U8 + SPI_BUFFER_NUM_DATA_BYTES];

static volatile uint8_t permanent_spi0_status = 0;
static volatile uint8_t current_spi0_status = 0;

static volatile bool byte_transmission_end = false;
static volatile uint16_t spi0_buffer_index = 0;
//static spi_response_t spi_response;

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
	#define SPI_SSEL_BIT_POS_U8 ((uint8_t)14U)
	
	/*Enable SPI alternative functions on pins : SCK, MISO, MOSI*/
	PINSEL0 |= (uint32_t)0x1500U;
	
	/*Also SSEL must be enabled even if it is not used*/
	PINSEL0 |= (uint32_t)((uint32_t)1U << SPI_SSEL_BIT_POS_U8);
	
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


void SPI0_irq (void) __irq
{
	/*Read current status which triggered interrupt*/
	current_spi0_status = (uint8_t)S0SPSR;
	
	/*Update permanent flag, to detect historical failures for diag module*/
	permanent_spi0_status |= current_spi0_status;
	
	/*Check if reason for interrupt is Byte Complete Transfer*/
	if(0 != ((uint8_t)S0SPSR & SPI_TRANSFER_COMPLETE_BIT_POS_MASK_U8))
	{
		/*Mark transfer as complete*/
		byte_transmission_end = true;
		
		/*Read current value returned from slave on current position. Increment of index will be done in SendMesage function*/
		spi0_TX_RX_buffer[spi0_buffer_index] = (uint8_t)S0SPDR;
	}
	
	/*Clear SPI Interrupt Flag*/
	S0SPINT = (uint8_t)1U;
	
	/*Acknowledge the interrupt for VIC module*/
	VICVectAddr = 0;
}

void spi0_sendReceive_message(uint8_t buffer[], const uint16_t length)
{
	/*Start at index 0 the SPI tranfer*/
	spi0_buffer_index = 0;
	
	/*Enable SSEL signal*/
	SPI0_set_slave_select(PIN_LOW);
	
	while(spi0_buffer_index < length)
	{
		/*Put current byte in data register in order to trigger transfer of current byte*/
		S0SPDR = buffer[spi0_buffer_index];
		
		/*Wait for ISR to mark end of current byte transmission*/
		while(false == byte_transmission_end)
		{
			/*To be added a timer and inserted here a check for interrupt pending. Function will return a timeout error if timeout occour later*/
		}
		
		/*reset byte sent flag for next byte transmission*/
		byte_transmission_end = false;
		
		buffer[spi0_buffer_index] = spi0_TX_RX_buffer[spi0_buffer_index];
		
		/*Increment buffer index*/
		spi0_buffer_index ++;
	}
	
	/*Disable SSEL signal*/
	SPI0_set_slave_select(PIN_HIGH);
}
