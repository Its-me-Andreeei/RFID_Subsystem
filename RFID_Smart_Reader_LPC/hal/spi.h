#ifndef __SPI_H
#define __SPI_H

#include <stdint.h>
#include <stdbool.h>

typedef enum process_request_t
{
	NO_REQUEST,
	REQUEST_COMPLETED,
	REQUEST_IN_PROGRESS
}process_request_t;

void SPI0_init(void);
void spi0_set_request(const uint8_t buffer[], const uint16_t length);
void spi0_get_response(uint8_t *out_buffer, uint16_t *length);
process_request_t spi0_get_request_status(void);
void spi0_process_request(void);
#endif
