#ifndef __Flash_IntelTE28F320C3_H
#define __Flash_IntelTE28F320C3_H

// Flash Intel TE28F320C3TD70
#include <stdint.h>


#define FLASH_WIDTH             16              // Effective Flash width (device width * FLASH_PAR_DEVICES)
#define FLASH_SEQ_DEV           1               // Number of sequenctial flash devices
#define FLASH_PAR_DEV           1               // Number of devices used to provide wider access width
#define FLASH_BLOCK_SIZE        0x8000           // Block Size in words 32Kwords

#define FLASH_BOOT              0x8000          //First 0x8000 bytes reserved for boot loader etc.
#define FLASH_SIZE              (0x400000-FLASH_BOOT)      // Total area of Flash region in words 8 bit
#define FLASH_BLOCK_COUNT	 	127
#define FLASH_BASE              0x80000000  //First 0x8000 bytes reserved for boot loader etc.
#define FLASH_BEGIN             FLASH_BASE
#define FLASH_END				(FLASH_BEGIN+FLASH_SIZE)

#define TRUE	1
#define FALSE	0
#define PASS	0
#define FAIL	-1

#define MAX_WRITE_BUFF		0x0F

typedef enum
{
  MANUFACTURER_ID = 0,
  DEVICE_ID = 1,
  BLOCK_LOCK_STATUS = 2,
  PROTECTION_REG_LOCK_STATUS = 0x80
} DEV_INDENT_CODE_DEF;

typedef enum
{
  UNKNOW_MAN_ID = 0,
  INTEL_MAN_ID,
  MXID_MAN,
} FLASH_DEV_MANUFACTURER_ID_DEF;

// Flash const



#define FLASH_ERROR 0x01
#define RAM_ERROR   0x02
#define UART_ERROR  0x04

// Intel Basic Command Set Commands
#define	READ_ID			0x90
#define PROGRAM_COMMAND		0x40		//or 0x10 both are valid
#define PROGRAM_VERIFY		0xD0
#define READ_STATUS		0x70
#define CLEAR_STATUS		0x50
#define READ_ARRAY		0xFF
#define BLOCK_ERASE		0x20
#define	PROG_ERASE_SUSPEND	0xB0
#define	PPROG_ERASE_RESUME	0xD0
#define BLOCK_LOCK_BITS		0x60
#define	PROT_PRG		0xC0
#define BLOCK_LOCK_CONFIRM	0x01
#define CFI_QUERY_COMMAND	0x98
#define CFI_DATA_OFFS		0x20
#define RESET_DEV		0xF0

// Word access
#define FIRST_KEY_ADD   	(0x555<<1)
#define SECOND_KEY_ADD   	(0x2AA<<1)
#define THIRD_KEY_ADD   	(0x555<<1)

#define KEY_AA           	0xAA
#define KEY_55           	0x55

#define BIT_PASS_W              0x00000080

// Intel Status masks
#define STATUS_LOCKED		0x02
#define LOCK_ERROR		0x10
#define ERASE_ERROR		0x20
#define STATUS_READY		0x80

uint16_t FLASH_INTEL_Read_ManufacturerID(void);
uint16_t FLASH_INTEL_Read_DeviceID(void);
uint32_t FLASH_INTEL_GetBlockStartAddress(uint8_t blockNumber);
uint8_t FLASH_INTEL_GetBlockNumber(uint32_t address);
int32_t FLASH_INTEL_Unlock_Block(uint16_t *address);
int32_t FLASH_INTEL_Erase_Block_At_Address(uint16_t *address);
int32_t FLASH_INTEL_Erase_Block(uint8_t blockNumber);
int32_t FLASH_INTEL_ChipErase(void);
int32_t FLASH_INTEL_ProgramWord(uint32_t programAddress, uint16_t data);
int32_t FLASH_INTEL_ProgramBuffer(uint32_t programAddress, uint8_t *buffer, uint32_t size);

#endif
