/*
 * PlatformTypes.h
 *
 * Created: 18.11.2023 21:05:43
 *  Author: Nitro5
 */ 


#ifndef PLATFORMTYPES_H_
#define PLATFORMTYPES_H_

#include <stdint-gcc.h>
typedef uint8_t uint8;
typedef uint8_t u8;

typedef uint16_t uint16;
typedef uint16_t u16;

typedef uint32_t uint32;
typedef uint32_t u32;

typedef uint8_t bool_t;

#define FALSE ((u8)0x00u)
#define TRUE ((u8)0x01u)

#define F_CPU   11059200UL


#endif /* PLATFORMTYPES_H_ */