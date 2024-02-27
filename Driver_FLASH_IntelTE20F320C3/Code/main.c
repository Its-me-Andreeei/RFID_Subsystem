#include <lpc22xx.h>
#include <stdio.h>
#include <stdint.h>
#include <Flash_IntelTE28F320C3/Flash_IntelTE28F320C3.h>
#include "global.h"
#include "memory.h"
#include "serial.h"
//#include "flash_test.h"

void WriteSRAM(unsigned long* Add, unsigned long uiData)
{
  *Add = uiData;
}

unsigned long ReadSRAM(unsigned long* Add)
{
  return *Add;
}

void TestSRAM()
{

  	unsigned long i       = 0;      //need for cycles
  	unsigned long* pAdd   = 0;      //pointer to SRAM
  //write to UART
	printf("Testing SRAM \n\r");

  //write to UART
	printf("Testing SRAM \n\r");

  //write data to SRAM
	printf("Writing data to SRAM \n\r");

	

	TIMER_RESET();

	TIMER_START();
 	for(i=0, pAdd=(unsigned long *)SRAM_BASE; pAdd < (unsigned long *)SRAM_BASE+SRAM_SIZE/4; ++pAdd,++i)
  	{
    	WriteSRAM(pAdd, i);
  	}

	TIMER_STOP();
	printf ("Write time: %X\n\r",T0TC);


  //read data from SRAM
  	TIMER_RESET();

  	printf("Reading data from SRAM \n\r");

	TIMER_START();

  	for(i=0, pAdd=(unsigned long *)SRAM_BASE; pAdd < (unsigned long *)SRAM_BASE+SRAM_SIZE/4; ++pAdd,++i)
  	{
     	if (*pAdd != i) break;
  	}

	TIMER_STOP();

	printf ("Read time: %X\n\r",T0TC);


  //compare read and write data
  //if data is same pAdd will reach last address
   	if (pAdd >= (unsigned long *)SRAM_BASE+SRAM_SIZE/4)
   	{
    	printf("Veryfied data is OK! \n\r");
   	}
   	else 
		printf("Veryfied is NOT OK! \n\r");

}
  


void error()
{
	IO0DIR |= 1 << 30;
	IO0CLR |= 1 << 30; 
	while(1);
}

void initEMC(void)
{
	PINSEL0 = 0;
	PINSEL1 = 0;
	PINSEL2 = 0x0F000924;
}

uint8_t FLASH_INTEL_TestMemory()
{
	uint16_t *address = (uint16_t*)(FLASH_BASE);
	uint8_t result;
	uint16_t data = 0;
	uint16_t readData = 0;
	uint32_t i = 0;
	
	printf ("Writing memory...");
	TIMER_RESET();
	TIMER_START();	
	
	for (i = 0; i < FLASH_SIZE / 2; i++, address++, data++)
	{
		result = FLASH_INTEL_ProgramWord((uint32_t)address, data);
		if (result != PASS)
		{
			printf ("error at %d %08X result %d - data to write %X data to read %X\n\r", i, address, result, data ,*address);
			return 0;
		}
	}
	TIMER_STOP();	
	printf ("done \n\r");
	printf ("Time: %08X\n\r", T0TC);

	printf ("Reading memory...");
	TIMER_RESET();
	TIMER_START();	
	
	for (i = 0, data = 0, address = (uint16_t*)(FLASH_BASE); i < FLASH_SIZE / 2; i++, data++, address++)
	{
		readData = *address;
		if (readData != data)
		{
			printf ("error at %d %08X \n\r", i, address);
			return 0;
		}
	}
	TIMER_STOP();
	printf ("done\n\r");
	printf ("Time: %08X\n\r", T0TC);
	
	return 1;
}

void TestFlashMemory()
{
	uint32_t manufacturerID = 0, deviceID = 0;	
	uint8_t result = 0;
	manufacturerID = FLASH_INTEL_Read_ManufacturerID();
	deviceID =  FLASH_INTEL_Read_DeviceID();
	printf ("Manufacturer ID:%04X\n\r", manufacturerID);
	printf ("Device ID:     :%04X\n\r", deviceID);

	printf ("Full Chip Erase...");
	TIMER_RESET();
	TIMER_START();	
	result = FLASH_INTEL_ChipErase();
	TIMER_STOP();
	printf ("done. Code %d\n\r", result);
	printf ("Time: %08X\n\r", T0TC);

	FLASH_INTEL_TestMemory();
}
#define N 10
uint8_t buffer[N];
uint8_t buffer_read[N];
uint8_t *p;
void TestFlashBufferWrite()
{
	uint32_t i = 0;
	uint8_t data = 0;
	uint32_t manufacturerID = 0, deviceID = 0;	
	int8_t result = 0;
	manufacturerID = FLASH_INTEL_Read_ManufacturerID();
	deviceID =  FLASH_INTEL_Read_DeviceID();
	printf ("Manufacturer ID:%04X\n\r", manufacturerID);
	printf ("Device ID:     :%04X\n\r", deviceID);
	printf ("Erase block 0...");
	FLASH_INTEL_Erase_Block(0);
	for (i = 0, data = 0; i < N; i++, data++)
	{
		buffer[i] = data;
	}
	result = FLASH_INTEL_ProgramBuffer(FLASH_BEGIN, buffer, N - 1);
	printf ("%d\n\r", result);
	p = (uint8_t*)FLASH_BEGIN;
	for (i = 0; i < N; i++)
	{
		buffer_read[i] = *p;
		p++;
	}
	printf ("%d\n\r", result);
}

int main(void)
{
	initEMC();
	initSerial();
	initTimer();
	printf ("\n\rHello\n\r");
	TestSRAM();
	TestFlashMemory();
	TestFlashBufferWrite();
	while (1);
	return 0;
}
