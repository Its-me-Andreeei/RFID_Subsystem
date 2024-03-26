#ifndef __READER_MANAGER_H
#define __READER_MANAGER_H
#include "../../mercuryapi-1.37.1.44/c/src/api/tm_reader.h"

#define TEMPERATURE_CHECK_TIMEOUT 10000
#define STATUS_NO_ON_ROUTE_TIMEOUT 2000
#define RESET_PIN_READER_U8 ((uint8_t)23U)

/*This is the maximum number of times we perform recovery sequences in case of lost communication*/
#define NUMBER_OF_RECOVERY_SEQUENCE_STEPS_U8 ((uint8_t)0x03U) 
/*This if the maximum number of PINGs we try after a hard reset is performed in sequence recovery process*/
#define NUMBER_OF_PING_CHECKS_AFTER_RESET_U8 ((uint8_t)4U)

typedef enum route_status_t{
	ROUTE_PENDING,
	ON_THE_ROUTE,
	NOT_ON_ROUTE
}route_status_t;

typedef enum ReaderRequest_t
{
	NO_REQUEST,
	READ_REQUEST_ASKED,
	REQUEST_IN_PROGRESS,
	REQUEST_FINISHED
}ReaderRequest_t;

void ReaderManagerInit(void);
void Reader_HW_Reset(void);
void Reader_Manager(void);

void Reader_Set_Read_Request(bool value);
route_status_t Reader_GET_route_status(void);
ReaderRequest_t Reader_GET_request_status(void);
bool Reader_SET_read_request(bool request);

#endif
