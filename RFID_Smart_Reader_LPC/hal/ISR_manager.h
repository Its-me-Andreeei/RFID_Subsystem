#ifndef __ISR_MANAGER_H
#define __ISR_MANAGER_H

#include <stdint.h>
#include "../utils/ringbuf.h"

extern tRingBufObject uart1_ringbuff_rx; /*TBD: To be handled through an interface function*/

void InitInterrupt(void);
#endif
