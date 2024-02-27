#include "Flash_IntelTE28F320C3.h"
#include <stdint.h>

#pragma push
#pragma O0

int8_t FLASH_INTEL_command(uint8_t command, uint16_t *address)
{
	*address = (uint16_t)command;
	return PASS;
}

int8_t FLASH_INTEL_ReadyWait(uint16_t* address, uint16_t data)
// This checks the status register, and loops untill the Flash device is 'ready'
{
	while((*address & STATUS_READY) != STATUS_READY);
	return PASS;
}

uint32_t FLASH_INTEL_FlashID(uint16_t* address)
{
	uint32_t hold;
	// Put device into 'READ_ID'.
	FLASH_INTEL_command(KEY_AA,(uint16_t*)(FIRST_KEY_ADD+FLASH_BEGIN));
	FLASH_INTEL_command(KEY_55,(uint16_t*)(SECOND_KEY_ADD+FLASH_BEGIN));
	FLASH_INTEL_command(READ_ID,(uint16_t*)(THIRD_KEY_ADD+FLASH_BEGIN));
	hold = *address;                // Read Data
	FLASH_INTEL_command (RESET_DEV, address);   // Reset 26LVXXXX
	FLASH_INTEL_command (READ_ARRAY, address);  // C3 Put device back into read mode.
	return(hold);
}

uint16_t FLASH_INTEL_Read_ManufacturerID(void)
{
 	return FLASH_INTEL_FlashID((uint16_t*)(FLASH_BASE) + MANUFACTURER_ID); //Read flash memory manifacturer
}

uint16_t FLASH_INTEL_Read_DeviceID(void)
{
	return FLASH_INTEL_FlashID((uint16_t*)(FLASH_BASE) + DEVICE_ID);  //Read flash memory manifacturer
}

int32_t FLASH_INTEL_Unlock_Block(uint16_t *address)
{
	FLASH_INTEL_command (READ_ARRAY, address);		// Put device back into read mode.
    FLASH_INTEL_command (BLOCK_LOCK_BITS, address);   	// Clear lock bits write 0x60 within the device address space
    FLASH_INTEL_command (PROGRAM_VERIFY, address);    	// Confirm action by writing 0xD0 within the device address space
	FLASH_INTEL_command (READ_ARRAY, address);		// Put device back into read mode.
	return PASS;
}

int32_t FLASH_INTEL_Erase_Block_At_Address(uint16_t *address)
// This function erases a single block, specified by it's base address
{
	int32_t result;
	uint32_t s_reg;
	FLASH_INTEL_Unlock_Block((uint16_t*)address);
	do
	{
		result = PASS;
		FLASH_INTEL_command(BLOCK_ERASE, (uint16_t*)address);	        // Issue single block erase command
		FLASH_INTEL_command(PROGRAM_VERIFY, (uint16_t*)address);	// Confirm Action 0xd0 with block address
		FLASH_INTEL_command(READ_STATUS, (uint16_t*)address);
		FLASH_INTEL_ReadyWait((uint16_t*)address,0xFFFF);	// Wait for erase attempt to complete
		s_reg = *(volatile uint32_t*)address;
		// This results in reading s_reg twice (first time in readywait), so Intel errata does not cause problem.
		if (s_reg & (ERASE_ERROR | (ERASE_ERROR<<16)))
		{
			FLASH_INTEL_command(CLEAR_STATUS, (uint16_t*)address);
			result = FAIL;
		}
		// Reset for Read operation
		FLASH_INTEL_command(READ_ARRAY, (uint16_t*)address);
	} while (result == FAIL);
	return (result);
}

int32_t FLASH_INTEL_Erase_Block(uint8_t blockNumber)
{
	if (blockNumber > 134)
	{
		return FAIL;
	}
	if (blockNumber >= 127)
	{
		return FLASH_INTEL_Erase_Block_At_Address((uint16_t*)(FLASH_BASE + 0x3F8000 + ((blockNumber - 127) * 0x1000)) );
	}
	else
	{
		return FLASH_INTEL_Erase_Block_At_Address((uint16_t*)FLASH_INTEL_GetBlockStartAddress(blockNumber));
	}
}

uint32_t FLASH_INTEL_GetBlockStartAddress(uint8_t blockNumber)
{
	if (blockNumber > FLASH_BLOCK_COUNT)
	{
		return 0xFFFFFFFF;
	}
	return FLASH_BASE + (blockNumber * FLASH_BLOCK_SIZE);
}

uint8_t FLASH_INTEL_GetBlockNumber(uint32_t address)
{
	if ((address < FLASH_BASE) && (address > FLASH_END))
	{
		return 0xFF;
	}
	address = address - FLASH_BASE;
	return address / FLASH_BLOCK_SIZE;
}

int32_t FLASH_INTEL_ChipErase(void)
{
	uint8_t i = 0;
	for (i = 0; i <= 134; i++)
	{
		printf ("Block %d \n\r", i);
		if (FLASH_INTEL_Erase_Block(i) == FAIL)
		{
			return FAIL;
		}				
	}
	
	return PASS;
}

int32_t FLASH_INTEL_ProgramWord(uint32_t programAddress, uint16_t data)
{
	uint16_t *address = (uint16_t*)programAddress;
    FLASH_INTEL_command(PROGRAM_COMMAND,(uint16_t*)address);	// Writing 0x40 puts devices into write mode.
	*((volatile unsigned short*)address) = data;    // Write data
    FLASH_INTEL_command (READ_STATUS,(unsigned short*)address);	// Put device into 'READ STATUS MODE'.
    FLASH_INTEL_ReadyWait((unsigned short*)address, 0);	// Wait for write to complete.
    while(*(volatile uint16_t*)address & STATUS_LOCKED) // Block was locked (if should suffice, but while good for debug).
    {
		FLASH_INTEL_Unlock_Block((uint16_t*)address);     // re-issue program command 0x10 (word)
        FLASH_INTEL_command(CLEAR_STATUS, (uint16_t*)address);  // clear status error
        FLASH_INTEL_command(PROGRAM_COMMAND, (uint16_t*)address);// address simply needs to be in the correct device
        *((volatile uint16_t*)address) = data;	// Write required word to required address
        FLASH_INTEL_command (READ_STATUS, (uint16_t*)address);	// Put device into 'READ STATUS MODE'.
        FLASH_INTEL_ReadyWait((uint16_t*)address, 0);		
    }
    FLASH_INTEL_command(READ_ARRAY, (uint16_t*)address);	// Reset for Read operation
    if (*address != data)                                 // Read word back to verify
	{
		return FAIL;
	}
	return PASS;
}

int32_t FLASH_INTEL_ProgramBuffer(uint32_t programAddress, uint8_t *buffer, uint32_t size)
{
	uint32_t i;
	uint16_t temp;
	uint32_t j = 0;
	uint32_t address = programAddress;
	if ((programAddress % 2) != 0)
	{
		return FAIL;
	}
	for (i = 0; i < size / 2; i++)
	{
		temp = 0;
		temp = (buffer[j + 1] << 8) | buffer[j];
		if (FLASH_INTEL_ProgramWord(address, temp) != PASS)
		{
			return FAIL;
		}
		address += 2;
		j += 2;		
	}
	if ( (size % 2) != 0)
	{
		temp = 0;
		temp = buffer[j] | (( (*(uint16_t*)address) & 0xFF) << 8);
		if (FLASH_INTEL_ProgramWord(address, temp) != PASS)
		{
			return FAIL;
		}
	}
	return PASS;
}
#pragma pop

