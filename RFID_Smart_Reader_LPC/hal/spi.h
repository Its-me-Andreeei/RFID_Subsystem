#ifndef __SPI_H
#define __SPI_H

#include <stdint.h>
#include <stdbool.h>


void SPI0_init(void);

void spi0_sendReceive_message(uint8_t buffer[], const uint16_t length);
#endif
