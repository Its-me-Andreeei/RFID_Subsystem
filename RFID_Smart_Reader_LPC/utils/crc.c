#include "crc.h"

static const  uint16_t crctable[16] = 
{
  0x0000, 0x1021, 0x2042, 0x3063,
  0x4084, 0x50a5, 0x60c6, 0x70e7,
  0x8108, 0x9129, 0xa14a, 0xb16b,
  0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
};

uint16_t compute_crc(const uint8_t *u8Buf, const uint8_t len)
{
  uint16_t crc;
  int i;

  crc = 0xffff;

  for (i = 0; i < len ; i++)
  {
    crc = ((crc << 4) | (u8Buf[i] >> 4))  ^ crctable[crc >> 12];
    crc = ((crc << 4) | (u8Buf[i] & 0xf)) ^ crctable[crc >> 12];
  }

  return crc;
}
