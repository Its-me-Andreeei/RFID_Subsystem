#include <LPC22xx.h>
#include <string.h>
#include <stdio.h>

#include "./spi.h"
#include "./spi_ISR.h"



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

typedef enum slave_ready_t
{
	SLAVE_READY,
	SLAVE_NOT_READY
}slave_ready_t;

typedef enum slave_select_state_t
{
	PIN_LOW = 0U,
	PIN_HIGH = 1U,
}slave_select_state_t;

typedef struct spi_response_t
{
	uint8_t response[SPI_BUFFER_NUM_DATA_BYTES];
	uint16_t response_length;
}spi_response_t;

/*Buffer will be filled with TX data which progressively will be filled with RX data*/
static uint8_t spi0_TX_RX_buffer[SPI_BUFFER_NUM_HEADER_BYTES_U8 + SPI_BUFFER_NUM_DATA_BYTES];
static uint8_t command[SPI_BUFFER_NUM_DATA_BYTES];
static uint16_t command_length;

static volatile uint8_t permanent_spi0_status = 0;
static volatile uint8_t current_spi0_status = 0;

static volatile bool handsake_detected = false;
static volatile bool byte_transmission_end = false;
static volatile uint16_t spi0_buffer_index = 0;

static process_request_t process_request_flag = NO_REQUEST;

static spi_response_t spi_response;

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

void spi0_get_response(uint8_t *out_buffer, uint16_t *length)
{
	if((out_buffer != NULL) && (length != NULL))
	{
		if(REQUEST_COMPLETED == process_request_flag)
		{
			memcpy(out_buffer, spi_response.response, spi_response.response_length);
			*length = spi_response.response_length;
			process_request_flag = NO_REQUEST;
		}
		else
		{
			*length = 0x0000;
		}
	}
}

void GPIO_Handsake_irq(void) __irq
{
	if(0 != ((uint8_t)EXTINT & ((uint8_t)1U << EINT2_POS_U8 )))
	{
		
		handsake_detected = true;
		printf("\nhs\n");
		
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


void spi0_set_request(const uint8_t buffer[], const uint16_t length)
{
	/*Check if size of request fits the buffer's size*/
	if(SPI_BUFFER_NUM_DATA_BYTES > length)
	{
		uint16_t index;
		for(index = 0; index < length; index++)
		{
			/*Copy input buffer to local buffer*/
			command[index] = buffer[index];
		}
		
		/*Copy input length into local length variable*/
		command_length = length;
		
		/*Set status to IN PROGRESS (this will be used in wifi_manager() )*/
		process_request_flag = REQUEST_IN_PROGRESS;
	}
}

static void spi0_sendReceive_message(const uint8_t buffer[], const uint16_t length)
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
		
		/*Increment buffer index*/
		spi0_buffer_index ++;
	}
	
	/*Disable SSEL signal*/
	SPI0_set_slave_select(PIN_HIGH);
}

process_request_t spi0_get_request_status(void)
{
	return process_request_flag;
}

void spi0_process_request(void)
{
	typedef enum comm_state_t
	{
		REQUEST_SEND_DATA,
		READ_DATA_STATUS_BEFORE_READ_WRITE,
		WRITE_DATA_TO_ESP,
		READ_DATA_FROM_ESP,
	}comm_state_t;
	
	static comm_state_t current_state = REQUEST_SEND_DATA;
	static uint8_t sequence_number = (uint8_t)0x01;
	
	/*Check if there is a request to send data*/
	if(REQUEST_IN_PROGRESS == process_request_flag)
	{
		switch(current_state)
		{
			case REQUEST_SEND_DATA:
			spi0_TX_RX_buffer[0] = (uint8_t)0x01U; /*Command byte*/
			spi0_TX_RX_buffer[1] = (uint8_t)0x00U; /*Address byte*/
			spi0_TX_RX_buffer[2] = (uint8_t)0x00U; /*Dummy Byte*/
		
			spi0_TX_RX_buffer[3] = (uint8_t)0xFEU;
			spi0_TX_RX_buffer[4] = sequence_number;
			spi0_TX_RX_buffer[5] = (uint8_t)(command_length & (uint8_t)0xFFU);
			spi0_TX_RX_buffer[6] = (uint8_t)((command_length >> (uint8_t)8) & (uint8_t)0xFFU);
			
			/*Send request and receive response*/
			spi0_sendReceive_message(spi0_TX_RX_buffer, (uint16_t)7U);/*TBD: Check Response*/
			
			current_state = READ_DATA_STATUS_BEFORE_READ_WRITE;
			break;
			
			case READ_DATA_STATUS_BEFORE_READ_WRITE:
				
			/*Wait for ESP to pull-up the handsake pin*/
			if(true == handsake_detected)
			{
				spi0_TX_RX_buffer[0] = (uint8_t)0x02U; /*Command byte*/
				spi0_TX_RX_buffer[1] = (uint8_t)0x04U; /*Address byte*/
				spi0_TX_RX_buffer[3] = (uint8_t)0x00;
				spi0_TX_RX_buffer[4] = (uint8_t)0x00;
				spi0_TX_RX_buffer[5] = (uint8_t)0x00;
				spi0_TX_RX_buffer[6] = (uint8_t)0x00;
				
				/*Send request and receive response*/
				spi0_sendReceive_message(spi0_TX_RX_buffer, (uint16_t)7U);
				
				/*Check if ESP is ready to receive data ( if it's writtable)*/
				if((uint8_t)0x02U == spi0_TX_RX_buffer[3])
				{
					current_state = WRITE_DATA_TO_ESP;
				}/*Check if ESP wants to send data ( it's readable)*/
				else if((uint8_t)0x01U == spi0_TX_RX_buffer[3])
				{
					/*Arrange command length bytes for reception*/
					command_length = (spi0_TX_RX_buffer[6] << (uint8_t)8) | spi0_TX_RX_buffer[5];
					current_state = READ_DATA_FROM_ESP;
				}
				else
				{
					/*TBD: Add error handling if wrong byte is received*/
				}
				
				/*Reset handsake flag for next transmission*/
				handsake_detected = false;
			}
			break;
			
			case WRITE_DATA_TO_ESP:
			spi0_TX_RX_buffer[0] = (uint8_t)0x03U;
			spi0_TX_RX_buffer[1] = (uint8_t)0x00;
			spi0_TX_RX_buffer[2] = (uint8_t)0x00;
			memcpy(spi0_TX_RX_buffer + SPI_BUFFER_NUM_HEADER_BYTES_U8, command, command_length);
			
			/*Send request and receive response*/
			spi0_sendReceive_message(spi0_TX_RX_buffer, SPI_BUFFER_NUM_HEADER_BYTES_U8 + command_length);
			
			/*TBD: to be checked for received response*/
			
			/*Send Write Done command*/
			spi0_TX_RX_buffer[0] = (uint8_t)0x07U;
			spi0_TX_RX_buffer[1] = 0x00;
			spi0_TX_RX_buffer[2] = 0x00;
			spi0_sendReceive_message(spi0_TX_RX_buffer, (uint16_t)3U);
			/*TBD: to be checked for received response*/
			
			current_state = READ_DATA_STATUS_BEFORE_READ_WRITE;
			break;
			
			case READ_DATA_FROM_ESP:
			spi0_TX_RX_buffer[0] = (uint8_t)0x04U;
			spi0_TX_RX_buffer[1] = (uint8_t)0x00;
			spi0_TX_RX_buffer[2] = (uint8_t)0x00;
			
			memset(spi0_TX_RX_buffer + SPI_BUFFER_NUM_HEADER_BYTES_U8, 0x00 ,command_length);
			
			/*Send request and receive response*/
			spi0_sendReceive_message(spi0_TX_RX_buffer, SPI_BUFFER_NUM_HEADER_BYTES_U8 + command_length);
			memcpy(command, spi0_TX_RX_buffer + SPI_BUFFER_NUM_HEADER_BYTES_U8, command_length);
				
			/*Send Read Done command*/
			spi0_TX_RX_buffer[0] = (uint8_t)0x08U;
			spi0_TX_RX_buffer[1] = 0x00;
			spi0_TX_RX_buffer[2] = 0x00;
			
			/*Send request and receive response*/
			spi0_sendReceive_message(spi0_TX_RX_buffer, (uint16_t)3U);
			
			/*Increment or reset sequence number*/
			if((uint8_t)0xFFU == sequence_number)
			{
				/*Reset sequence counter in order to avoid implicit overflow*/
				sequence_number = (uint8_t)0x00U;
			}
			else
			{
				/*Increment sequence counter, if there is no overflow risk*/
				sequence_number++;
			}
			
			/*Reset finite state machine for next transaction*/
			current_state = REQUEST_SEND_DATA;
			
			/*Reset flag for new requests to be made. This flag is protecting sequence from being overriten by other requests, until current request is done*/
			process_request_flag = REQUEST_COMPLETED;
			break;
			
			default:
			current_state = REQUEST_SEND_DATA;
			break;
		}
	}
}
