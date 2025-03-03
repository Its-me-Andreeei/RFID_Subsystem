#include "flash_stm29w320db.h"
#include <lpc22xx.h>
#include <global.h>
#include <stdio.h>

FLASH_ST_COMMAND Flash_Command_AutoSelect =	{ 
												{0x555, 0x2AA, 0x555},
												{0x0AA, 0x055, 0x090},
												3
											};

FLASH_ST_COMMAND Flash_Command_Reset =		{ 
												{0x000},
												{0x0F0},
												1
											};

FLASH_ST_COMMAND Flash_Command_Read =		{ 
												{0x555, 0x2AA, 0x000},
												{0x0AA, 0x055, 0x0F0},
												3
											};

FLASH_ST_COMMAND Flash_Command_ChipErase = 	{
												{0x555, 0x2AA, 0x555, 0x555, 0x2AA, 0x555},
												{0x0AA, 0x055, 0x080, 0x0AA, 0x055, 0x010},
												6
											};

FLASH_ST_COMMAND Flash_Command_Program =	{
												{0x555, 0x2AA, 0x555},
												{0x0AA, 0x055, 0x0A0},
												3,
											};

FLASH_ST_COMMAND Flash_Command_BlockErase = {
												{0x555, 0x2AA, 0x555, 0x555, 0x2AA},
												{0x0AA, 0x055, 0x080, 0x0AA, 0x055},
												5,
											};


uint16_t testBuffer[32];

void FLASH_ST_SendCommand(FLASH_ST_COMMAND *command)
{
	uint8_t i = 0;
	uint16_t *address;
	for (i = 0; i < command->CommandSize; i++)
	{
		address = ((uint16_t*)(FLASH_START + (command->CommandAddr[i] << 1)));
		*address = command->CommandData[i];
	}
}

uint16_t FLASH_ST_Read_FlashID(uint16_t *address)
{
	uint16_t result = 0;
	FLASH_ST_SendCommand(&Flash_Command_AutoSelect);
	result = *address;
	FLASH_ST_SendCommand(&Flash_Command_Reset);
	FLASH_ST_SendCommand(&Flash_Command_Read);
	return result;
}

uint16_t FLASH_ST_Read_ManufacturerID()
{
	return FLASH_ST_Read_FlashID((uint16_t*)(FLASH_BASE));
}

uint16_t FLASH_ST_Read_DeviceID()
{
	return FLASH_ST_Read_FlashID((uint16_t*)(FLASH_BASE) + 1);
}

uint8_t FLASH_ST_ChipErase()
{	
	uint8_t result = 0xFF;
	uint8_t currentByte = 0xFF;
	uint8_t lastByte = 0xFF;
	uint16_t *address;
	address = (uint16_t*)(FLASH_BASE);
	FLASH_ST_SendCommand(&Flash_Command_ChipErase);
	while(1)
	{
		currentByte = (*address) & 0xFF;
		if ((currentByte == lastByte) || (currentByte == 0xFF))
		{
			result = 1;
			break;
		}
		else
		{
			if ((currentByte & BIT7) != 0)
			{
				result = 2;
				break;
			}
			if ((lastByte != 0xFF) && ((currentByte & BIT6) == (lastByte & BIT6)))
			{
				result = 3;
				break;
			}
			if ((currentByte & BIT3) == 0)
			{
				result = 0;
				break;
			}
		}
		lastByte = currentByte;
	}
	if (result != 1)
	{
		FLASH_ST_SendCommand(&Flash_Command_Reset);
		FLASH_ST_SendCommand(&Flash_Command_Read);
	}
	return result;
}

uint8_t FLASH_ST_ProgramWord(uint32_t programAddress, uint16_t programData)
{
	volatile uint16_t i = 0;
	volatile uint8_t result = 0xFF;
	volatile uint8_t currentByte = 0xFF;
	volatile uint16_t currentData = 0xFFFF;
	volatile uint8_t lastByte = 0xFF;
	volatile uint16_t *address;
	address = (uint16_t*)(programAddress);	
	FLASH_ST_SendCommand(&Flash_Command_Program);
	*address = programData;
	
	while (1)
	{
		currentData = *address;
		currentByte = currentData & 0xFF;
		testBuffer[i] = currentByte;
		i = (i + 1) % 32;

		if ((currentByte == lastByte) || (currentData == programData))
		{
			result = 1;
			break;
		}
		else
		{
			if ((currentByte & BIT7) == (programData & BIT7))
			{
				result = 2;
				break;
			}

			if ((currentByte & BIT5) != 0)
			{
				result = 0;
				break;
			}
			/*
			if ((currentByte & BIT6) == (lastByte & BIT6))
			{
				result = 4;
				break;
			}
			*/
		}
		lastByte = currentByte;
	}
	if (result != 1)
	{
		FLASH_ST_SendCommand(&Flash_Command_Reset);
		FLASH_ST_SendCommand(&Flash_Command_Read);
		if (*address != programData)
		{
			return result;
		}
		else
		{
			return 1;
		}
	}	
	return result;
}

uint32_t FLASH_ST_GetBlockStartAddress(uint8_t blockNumber)
{
	if (blockNumber > FLASH_BLOCK_COUNT)
	{
		return 0xFFFFFFFF;
	}
	return FLASH_BASE + (blockNumber * FLASH_BLOCK_SIZE);
}

uint8_t FLASH_ST_GetBlockNumber(uint32_t address)
{
	if ((address < FLASH_BASE) && (address > FLASH_END))
	{
		return 0xFF;
	}
	address = address - FLASH_BASE;
	return address / FLASH_BLOCK_SIZE;
}
