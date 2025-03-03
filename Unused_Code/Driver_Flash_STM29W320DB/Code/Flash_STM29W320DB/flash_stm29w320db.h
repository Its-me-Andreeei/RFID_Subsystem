#ifndef __FLASH_STM29W320DB
#define __FLASH_STM29W320DB

#include <stdint.h>

#define FLASH_BOOT							0x8000

#define FLASH_SIZE     						(0x400000-FLASH_BOOT)      				// Total area of Flash region in words 8 bit
#define FLASH_START    						0x80000000       						// First 0x10000 bytes reserved for boot loader etc.
#define FLASH_END							(FLASH_START+FLASH_SIZE+FLASH_BOOT)		// Flash end address
#define FLASH_BASE							(FLASH_START+FLASH_BOOT)				// Flash usable base address
#define FLASH_BLOCK_COUNT	 				63
#define FLASH_BLOCK_SIZE					0x8000									// Block size 64 KB

#define FLASH_ST_COMMAND_MAX_LENGTH	8

typedef struct
{
	uint16_t CommandAddr[FLASH_ST_COMMAND_MAX_LENGTH];
	uint16_t CommandData[FLASH_ST_COMMAND_MAX_LENGTH];	
	uint8_t CommandSize;
}FLASH_ST_COMMAND;

uint16_t FLASH_ST_Read_FlashID(uint16_t *address);
void FLASH_ST_SendCommand(FLASH_ST_COMMAND *command);

uint32_t FLASH_ST_GetBlockStartAddress(uint8_t blockNumber);
uint16_t FLASH_ST_Read_ManufacturerID(void);
uint16_t FLASH_ST_Read_DeviceID(void);
uint8_t FLASH_ST_ChipErase(void);
uint8_t FLASH_ST_ProgramWord(uint32_t programAddress, uint16_t programData);
uint8_t FLASH_ST_GetBlockNumber(uint32_t address);

#endif
