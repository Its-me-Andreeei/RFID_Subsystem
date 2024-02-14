#include "reader_manager.h"
#include <LPC22xx.h>
#include "../../utils/timer_software.h"
#include "../../mercuryapi-1.37.1.44/c/src/api/tm_reader.h"
#include <stdio.h>

#ifndef NULL
#define NULL ((void*) 0)
#endif

static TMR_Reader reader;
static TMR_TagReadData data;

typedef enum ReaderManager_state_en{
	MODULE_INIT,
	START_READING,
	STOP_READING,
	GET_TAGS, /*This will be default state*/
	CHECK_TEMPERATURE,
	ERROR_MANAGEMENT
}ReaderManager_state_en;

static void setRegion_NA2(void)
{
	TMR_Status status;
	const TMR_Region region = TMR_REGION_NA2;
	status = TMR_paramSet(&reader, TMR_PARAM_REGION_ID, &region);
	return;
}
	
static void setReadPlan_Simple(void)
{
	TMR_Status status;
	TMR_ReadPlan readPlan;
	uint8_t antennaList[] = {(uint8_t)1U};
	const uint8_t antennaCount = 1;
	const TMR_TagProtocol protocol = TMR_TAG_PROTOCOL_GEN2;
	TMR_RP_init_simple(&readPlan, antennaCount, antennaList,protocol, 100U);
	status = TMR_paramSet(&reader, TMR_PARAM_READ_PLAN, &readPlan);
	return;
}

static void setCompleteMetadataFlag(void)
{
	TMR_Status status;
	TMR_TRD_MetadataFlag metadata = (uint16_t)(TMR_TRD_METADATA_FLAG_ALL & (~TMR_TRD_METADATA_FLAG_TAGTYPE));
	status = TMR_paramSet(&reader, TMR_PARAM_METADATAFLAG, &metadata);
	return;
}

static void setTagFilter_Off(void)
{
	bool readFilter = false;
	TMR_paramSet(&reader, TMR_PARAM_TAGREADDATA_ENABLEREADFILTER, &readFilter);
}

static void setRfTime_OnOff(void)
{
	TMR_Status status;
	uint16_t asyncOnTime= (uint16_t) 500; /*Time in ms*/
	uint16_t asyncOffTime= (uint16_t) 1000; /*Time in ms*/
	
	status = TMR_paramSet(&reader, TMR_PARAM_READ_ASYNCONTIME, &asyncOnTime);
	status = TMR_paramSet(&reader, TMR_PARAM_READ_ASYNCOFFTIME, &asyncOffTime);
	
	return;
}

void ReaderManagerInit(void)
{
	(void)TMR_SR_SerialTransportDummyInit(&reader.u.serialReader.transport, NULL, NULL);
	
	TMR_create(&reader, "eapi:///UART1");
	TMR_connect(&reader);

	setRegion_NA2();
	setReadPlan_Simple();
	setCompleteMetadataFlag();
	setTagFilter_Off();
	setRfTime_OnOff();
}

void Reader_Reset(void)
{
	IO0CLR = (uint8_t)1 << (uint8_t)23;
	TIMER_SOFTWARE_Wait(1000);
	IO0SET = (uint8_t)1 << (uint8_t)23;
	TIMER_SOFTWARE_Wait(1000);
}

void Reader_Manager(void)
{
	static ReaderManager_state_en current_state_en = START_READING;
	
	switch(current_state_en)
	{
		/*We assume init is already done. We only get here by other internal states request*/
		case MODULE_INIT:
			ReaderManagerInit();
			current_state_en = START_READING;
			break;
		
		case START_READING:
			TMR_startReading(&reader);
			current_state_en = GET_TAGS;
			break;
		
		case STOP_READING:
			TMR_stopReading(&reader);
			break;
		
		case GET_TAGS:
			if(TMR_SUCCESS == TMR_hasMoreTags(&reader))
			{
				TMR_getNextTag(&reader, &data);
				printf("EPC:" );
				for(uint16_t i = 0; i< data.tag.epcByteCount; i++)
				{
					printf("%02X ", data.tag.epc[i]);
				}
				printf("\n");
			}
			current_state_en = GET_TAGS;
			break;
		
		case CHECK_TEMPERATURE:
			/*TBD: to be cheched for reader temperature, for safety meeasures*/
			break;
		
		default:
			/*Invalid state reached, go back to 'GET TAGS' state*/
			current_state_en = GET_TAGS;
			break;
	}


}
