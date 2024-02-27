/*
 * $Revision: 1.0 $
 */

#ifndef __FLASH_LPC
#define __FLASH_LPC

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

#define FLASH_BOOT              0x10000          //First 0x8000 bytes reserved for boot loader etc.

//#define FLASH_SIZE              0x10000         // Total area of Flash region in words 8 bit
#define FLASH_WIDTH             16              // Effective Flash width (device width * FLASH_PAR_DEVICES)
#define FLASH_SEQ_DEV           1               // Number of sequenctial flash devices
#define FLASH_PAR_DEV           1               // Number of devices used to provide wider access width
#define FLASH_BLOCK             32768           // Block Size in words 32Kwords



#define FLASH_SIZE              (0x80000-FLASH_BOOT)      // Total area of Flash region in words 8 bit
#define FLASH_BEGIN             0x80000000
#define FLASH_BASE              (FLASH_BEGIN+FLASH_BOOT)  //First 0x8000 bytes reserved for boot loader etc.

#define FLASH_ERROR 0x01
#define RAM_ERROR   0x02
#define UART_ERROR  0x04

int command(unsigned char command, unsigned short *address);
int ReadyWait(unsigned short* address,unsigned short data, FLASH_DEV_MANUFACTURER_ID_DEF DevID);
int FlashID(unsigned short* address);
int Flash_Unlock_Block(unsigned short *address,FLASH_DEV_MANUFACTURER_ID_DEF DevID);
int Flash_Write_Word(unsigned long *address, unsigned long data, FLASH_DEV_MANUFACTURER_ID_DEF DevID);
int Flash_Write_Area(unsigned long *address, long *data, long size, FLASH_DEV_MANUFACTURER_ID_DEF DevID);
int Flash_Erase_Block(unsigned long *address, FLASH_DEV_MANUFACTURER_ID_DEF DevID);
int Flash_Erase_Blocks(unsigned long *address, long size, FLASH_DEV_MANUFACTURER_ID_DEF DevID);


void TestFLASH(void);


#endif // __FLASH_LPC
