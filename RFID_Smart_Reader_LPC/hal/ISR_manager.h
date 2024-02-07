#ifndef __ISR_MANAGER_H
#define __ISR_MANAGER_H

#include <stdint.h>
#include "../utils/ringbuf.h"

extern tRingBufObject uart1_ringbuff_rx;

void InitInterrupt(void);
#endif
