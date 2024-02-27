#ifndef __MEMORY_H
#define __MEMORY_H

#define SRAM_SIZE     0x100000         // Total area of Flash region in words 8 bit
#define SRAM_BASE     0x81000000       // First 0x8000 bytes reserved for boot loader etc.

#define BASE_SRAM 				(*( (unsigned char *) SRAM_BASE))


#endif

