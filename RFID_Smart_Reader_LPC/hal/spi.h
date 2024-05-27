#ifndef __SPI_H
#define __SPI_H

#include "../PlatformTypes.h"


void SPI0_init(void);

void spi0_sendReceive_message(u8 buffer[], const u16 length);
#endif
