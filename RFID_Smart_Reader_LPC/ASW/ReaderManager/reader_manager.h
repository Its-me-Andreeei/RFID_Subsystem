#ifndef __READER_MANAGER_H
#define __READER_MANAGER_H
#include "../../PlatformTypes.h"

#define STATUS_NO_ON_ROUTE_TIMEOUT 3000
#define PANIC_TIMEOUT							 5000
#define RESET_PIN_READER_U8 ((uint8_t)23U)

/*This is the maximum number of times we perform recovery sequences in case of lost communication*/
#define NUMBER_OF_RECOVERY_SEQUENCE_STEPS_U8 ((uint8_t)0x02U) 
/*This if the maximum number of PINGs we try after a hard reset is performed in sequence recovery process*/
#define NUMBER_OF_PING_CHECKS_AFTER_RESET_U8 ((uint8_t)4U)

typedef enum ReaderRequest_t
{
	NO_REQUEST,
	READ_REQUEST_ASKED,
	REQUEST_IN_PROGRESS,
	REQUEST_FINISHED,
	READER_IN_FAILURE
}ReaderRequest_t;

void ReaderManagerInit(void);
void Reader_HW_Reset(void);
void Reader_Manager(void);

ReaderRequest_t Reader_GET_request_status(void);
bool Reader_SET_read_request(bool request);
bool Reader_GET_internal_failure_status(void);
bool Reader_GET_TagInformation(u8 *data, u8 *len);

#endif
