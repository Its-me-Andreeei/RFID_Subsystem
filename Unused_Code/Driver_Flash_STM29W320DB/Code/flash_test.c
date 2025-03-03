/*
 * $Revision: 1.0 $
 */

#include "flash_test.h"
//#include "uart.h"
#include <stdio.h>

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


int command(unsigned char command, unsigned short *address)
{
  *address = (unsigned short)command;
  return PASS;
}

int ReadyWait(unsigned short* address,unsigned short data, FLASH_DEV_MANUFACTURER_ID_DEF DevID)
// This checks the status register, and loops untill the Flash device is 'ready'
{
  if (DevID == INTEL_MAN_ID)
  {
    while((*address & STATUS_READY)!=STATUS_READY);
    return PASS;
  }
  else if (DevID == MXID_MAN)
  {
    while ((data ^ *address) & BIT_PASS_W);
    if ((data ^ *address) & BIT_PASS_W)
      return(FAIL);
    return(PASS);
  }
  return(FALSE);
}

int FlashID(unsigned short* address)
{
unsigned int hold;
  // Put device into 'READ_ID'.
  command(KEY_AA,(unsigned short*)(FIRST_KEY_ADD+FLASH_BEGIN));
  command(KEY_55,(unsigned short*)(SECOND_KEY_ADD+FLASH_BEGIN));
  command(READ_ID,(unsigned short*)(THIRD_KEY_ADD+FLASH_BEGIN));
  hold = *address;                // Read Data
  command (RESET_DEV, address);   // Reset 26LVXXXX
  command (READ_ARRAY, address);  // C3 Put device back into read mode.
// Return Item
  return(hold);
}

unsigned short status;
int Flash_Unlock_Block(unsigned short *address, FLASH_DEV_MANUFACTURER_ID_DEF DevID)
{
//  do
//  {
    command (READ_ARRAY, address);		// Put device back into read mode.
    command (BLOCK_LOCK_BITS, address);   	// Clear lock bits write 0x60 within the device address space
    command (PROGRAM_VERIFY, address);    	// Confirm action by writing 0xD0 within the device address space
//    command (READ_ID, address);    	        // Confirm action by writing 0xD0 within the device address space
//    status = *(volatile unsigned short*)(address + BLOCK_LOCK_STATUS);
//  }
//  while(status);
//  ReadyWait(address,0xFFFF,DevID);   		// Wait for write to complete.
  command (READ_ARRAY, address);		// Put device back into read mode.
  return PASS;
}

int Flash_Write_Word(unsigned long *address, unsigned long data, FLASH_DEV_MANUFACTURER_ID_DEF DevID)
// This routine writes a single 32 bit word to Flash.  On entry, it assumes the
// Flash is in READ-ARRAY mode (which it is following reset, or exit from any other
// non-static function in this source file.
// For 16-bit access flash, Little Endian is presumed.
// This function checks whether the block is locked before attempting the write.
{
  // ***LPC2214 IS LITTLE ENDIAN***

int i;
unsigned short shdata;
  shdata=(short)data;
  if(DevID == INTEL_MAN_ID)
  {
    for (i=0;i<2;i++)
    {
      command(PROGRAM_COMMAND,(unsigned short*)address);	// Writing 0x40 puts devices into write mode.
      *((volatile unsigned short*)address+i) = shdata;    // Write data
      command (READ_STATUS,(unsigned short*)address);	// Put device into 'READ STATUS MODE'.
      ReadyWait((unsigned short*)address,0,INTEL_MAN_ID);	// Wait for write to complete.
      while(*(volatile unsigned short*)address & STATUS_LOCKED) // Block was locked (if should suffice, but while good for debug).
      {
        Flash_Unlock_Block((unsigned short*)address,INTEL_MAN_ID);     // re-issue program command 0x10 (word)
        command(CLEAR_STATUS, (unsigned short*)address);  // clear status error
        command(PROGRAM_COMMAND, (unsigned short*)address+i);// address simply needs to be in the correct device
        *((volatile unsigned short*)address+i) = shdata;	// Write required word to required address
        command (READ_STATUS, (unsigned short*)address);	// Put device into 'READ STATUS MODE'.
        ReadyWait((unsigned short*)address,0,INTEL_MAN_ID);		
      }
      shdata = data >> 16;
    }

    command(READ_ARRAY, (unsigned short*)address);	// Reset for Read operation
    if (*address != data)                                 // Read word back to verify
      return FAIL;
  }
  else if (DevID == MXID_MAN)
  {
    for (i=0;i<2;i++)
    {
      /* Flash Write Command */
      command(KEY_AA,(unsigned short*)(FIRST_KEY_ADD+FLASH_BEGIN));
      command(KEY_55,(unsigned short*)(SECOND_KEY_ADD+FLASH_BEGIN));
      command(0xA0,(unsigned short*)(THIRD_KEY_ADD+FLASH_BEGIN));
      *((volatile unsigned short*)address+i) = shdata;	// Write required word to required address
      if(ReadyWait((unsigned short*)address+i,shdata,MXID_MAN) != PASS){	/* Check Write End */
        return FAIL;
      }
      shdata = data >> 16;
    }
  }
  else
    return FALSE;
  return PASS;
}

int Flash_Write_Area(unsigned long *address, long *data, long size, FLASH_DEV_MANUFACTURER_ID_DEF DevID)
// This function writes an arbitary area of flash.
// It first verfies that the data will fit within the flash address space,
// Then divides the operation into calls to Flash_Write_Word, which writes
// data areas contained within a single block.
{
	unsigned int i = 0;
  if ((address >= (unsigned long*)FLASH_BASE) && ((address+size/4) <= (unsigned long*)(FLASH_BASE + FLASH_SIZE)))
  {
    for (i = 0; i!=size; ++i)
    {
      if(Flash_Write_Word(address,*(data+i),DevID)!=PASS) return FAIL;
    }
  }
  else
  {
    return FAIL;
  }
  return PASS;
}


int Flash_Erase_Block(unsigned long *address, FLASH_DEV_MANUFACTURER_ID_DEF DevID)
// This function erases a single block, specified by it's base address
{
int result;
unsigned int s_reg;
  if (DevID == INTEL_MAN_ID)
  {
    Flash_Unlock_Block((unsigned short*)address,INTEL_MAN_ID);
    do
    {
      result = PASS;
      command(BLOCK_ERASE, (unsigned short*)address);	        // Issue single block erase command
      command(PROGRAM_VERIFY, (unsigned short*)address);	// Confirm Action 0xd0 with block address
      command(READ_STATUS, (unsigned short*)address);
      ReadyWait((unsigned short*)address,0xFFFF,INTEL_MAN_ID);	// Wait for erase attempt to complete
      s_reg = *(volatile unsigned int*)address;
      // This results in reading s_reg twice (first time in readywait), so Intel errata does not cause problem.
      if (s_reg & (ERASE_ERROR | (ERASE_ERROR<<16)))
      {
        command(CLEAR_STATUS, (unsigned short*)address);
        result = FAIL;
      }
      // Reset for Read operation
      command(READ_ARRAY, (unsigned short*)address);
    } while (result == FAIL);
  }
  else
  {
    command(KEY_AA,(unsigned short*)(FIRST_KEY_ADD+FLASH_BEGIN));
    command(KEY_55,(unsigned short*)(SECOND_KEY_ADD+FLASH_BEGIN));
    command(0x80,(unsigned short*)(THIRD_KEY_ADD+FLASH_BEGIN));
    command(KEY_AA,(unsigned short*)(FIRST_KEY_ADD+FLASH_BEGIN));
    command(KEY_55,(unsigned short*)(SECOND_KEY_ADD+FLASH_BEGIN));
    command(0x30,(unsigned short*)address);
    result = ReadyWait((unsigned short*)address,0xFFFF,MXID_MAN);
  }
  return (result);
}

int Flash_Erase_Blocks(unsigned long *address, long size, FLASH_DEV_MANUFACTURER_ID_DEF DevID)
// This function erases all blocks overlapping specified region (via calls to Flash_Erase_Block)
{
unsigned long* limit = address+size/4-1;
  for(;address<=limit;address+=FLASH_BLOCK/4)
   Flash_Erase_Block(address,DevID);
  return PASS;
}


void TestFLASH()
{
  unsigned long i         = 0;      //need for cycles
  unsigned long* pAdd     = 0;      //pointer to SRAM
  unsigned int id         = 0;      //for flash identification

  unsigned int err_status_erase = 0;      //eracing flash status
  unsigned int err_status_write = 0;      //writing flash status
  FLASH_DEV_MANUFACTURER_ID_DEF FlashDevManID = UNKNOW_MAN_ID;

   printf("Testing Flash Memory \n\r");




  //read manifacturer
  printf("Read manufacturer \n\r");
  id = FlashID((unsigned short*)(FLASH_BASE)+MANUFACTURER_ID); //Read flash memory manifacturer
  switch(id)
  {
    case 0x0089: printf("Intel flash device is find \n\r");  FlashDevManID = INTEL_MAN_ID;   break;
    case 0x00C2: printf("MXIC flash device is find \n\r");   FlashDevManID = MXID_MAN;       break;
        default: printf("Unknow flash device is find \n\r"); break;
  }
	
  printf ("ManufacturerID: %X\n\r",id);

  // Read type of flash memory
  printf("Read type of chip\n\r");
  id = FlashID((unsigned short*)(FLASH_BASE)+DEVICE_ID);  //Read flash memory manifacturer
  printf ("DeviceID: %X\n\r",id);
  switch(id)
  {
    case 0x88C5: printf("TE28F320C3Bxxx\n\r"); break;
    case 0x88C4: printf("TE28F320C3Bxxx\n\r"); break;
    case 0x00C2: printf("MX26LV800Bxxx\n\r");  break;
    case 0x225B: printf("MX26LV800Bxxx\n\r");  break;
        default: printf("Unknow flash type \n\r");
                 FlashDevManID = UNKNOW_MAN_ID;  break;
  }

  if (FlashDevManID != UNKNOW_MAN_ID)
  {
    //Eracing flash
    printf("Eracing flash... \n\r");
    for(i=8, pAdd=(unsigned long *)FLASH_BASE; pAdd < (unsigned long *)FLASH_BASE+FLASH_SIZE/4; pAdd += FLASH_BLOCK/4,++i)
    {
      if (Flash_Erase_Block(pAdd,FlashDevManID) != PASS) break;
    }

    //if flash is erased pAdd will reach last address
    if (pAdd >= (unsigned long *)FLASH_BASE+FLASH_SIZE/4)
    {
      printf("Flash Erased! \n\r");
    }
    else
    {
      printf("Flash Erase fault\n\r");
      err_status_erase = 1;
    }

    //if flash is eracing -> write flash
    if(err_status_erase != 1)
    {
      printf("Flash Writing...\n\r");
      for(i=0, pAdd=(unsigned long *)FLASH_BASE; i < FLASH_SIZE/4; ++pAdd,++i)
      {
        if (Flash_Write_Word(pAdd,i,FlashDevManID) != PASS) break;
      }
      if (pAdd >= (unsigned long *)FLASH_BASE+FLASH_SIZE/4)
      {
        printf("Flash Write! \n\r");
      }
      else
      {
        printf("Flash Write Fault!\n\r");
        err_status_write = 1;
      }

      if(err_status_write != 1)
      {
        printf("Verifing Flash data... \n\r");

        for(i=0, pAdd=(unsigned long *)FLASH_BASE; i < FLASH_SIZE/4; ++pAdd,++i)
        {
          if (*pAdd != i) break;
        }

        if (pAdd >= (unsigned long *)FLASH_BASE+FLASH_SIZE/4)
        {
          printf("Verified flas is OK! \n\r");
        }
        else
        {
          printf("Verified flas is NOT OK \n\r");
        }
      }
    }
  }
  else
  {
    printf("Unknow type flash device is find. Test fault");
  }
}
