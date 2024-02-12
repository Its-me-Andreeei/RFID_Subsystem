#ifndef __ISR_MANAGER_H
#define __ISR_MANAGER_H

#include <stdint.h>
#include "../utils/ringbuf.h"

extern tRingBufObject uart1_ringbuff_rx;
extern volatile uint32_t RxCnt;

void InitInterrupt(void);
#endif
