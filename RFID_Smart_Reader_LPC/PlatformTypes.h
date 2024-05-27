#ifndef __PLATFORMTYPES_H
#define __PLATFORMTYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum state_t
{
	STATE_OK,
	STATE_NOK,
	STATE_PENDING,
	STATE_INVALID
}state_t;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#endif
