#include <lpc22xx.h>
#include <stdio.h>
#include "hal/timer.h"
#include "hal/uart0.h"
#include "utils/timer_software.h"
#include "hal/ISR_manager.h"
#include "mercuryapi-1.37.1.44/c/src/api/tm_reader.h"
#include "hal/uart1.h"

static TMR_Reader reader;
TMR_TagReadData data;
TMR_Status status;

void setRegion_NA2(void)
{
	const TMR_Region region = TMR_REGION_NA2;
	status = TMR_paramSet(&reader, TMR_PARAM_REGION_ID, &region);
	return;
}

void setReadPlan_Simple(void)
{
	TMR_ReadPlan readPlan;
	uint8_t antennaList[1] = {(uint8_t)1U};
	TMR_RP_init_simple(&readPlan, (uint8_t)1U, antennaList,TMR_TAG_PROTOCOL_GEN2, 1000U);
	status = TMR_paramSet(&reader, TMR_PARAM_READ_PLAN, &readPlan);
	return;
}

void setCompleteMetadataFlag(void)
{
	TMR_TRD_MetadataFlag metadata = (uint16_t)(TMR_TRD_METADATA_FLAG_ALL & (~TMR_TRD_METADATA_FLAG_TAGTYPE));
	status = TMR_paramSet(&reader, TMR_PARAM_METADATAFLAG, &metadata);
	return;
}

int main(void)
{	 
	timer_software_handler_t timer1;
	TIMER_SOFTWARE_init();
	TIMER_Init(); /*This is the HW timer*/
	UART0_Init(); /*Debugging port*/
	UART1_Init();
	InitInterrupt();
	
	printf("Main\n");
	/*------------------------------------*/
	(void)TMR_SR_SerialTransportDummyInit(&reader.u.serialReader.transport, NULL, NULL);
	//TMR_SR_SerialReader_init(&reader);
	TMR_create(&reader, "eapi:///UART1");
	status = TMR_connect(&reader);
	
	setRegion_NA2();
	setReadPlan_Simple();
	setCompleteMetadataFlag();
	/*------------------------------------*/
	
	timer1 = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer1, MODE_0, 3000, 1);
	TIMER_SOFTWARE_reset_timer(timer1);
	TMR_startReading(&reader);
	/*------------------------------------*/
	TIMER_SOFTWARE_start_timer(timer1);
	while(1)
	{

		/*
		while(!RingBufEmpty(&uart1_ringbuff_rx))
			printf("%02X ", RingBufReadOne(&uart1_ringbuff_rx));*/
		
		if(TMR_SUCCESS == TMR_hasMoreTags(&reader))
		{
			TMR_getNextTag(&reader, &data);
			printf("EPC: \n");
			for(uint8_t i = 0; i< data.tag.epcByteCount; i++)
			{
				printf("%02X ", data.tag.epc[i]);
			}
			printf("\n");
		}
		if (TIMER_SOFTWARE_interrupt_pending(timer1))
		{ 
			TMR_stopReading(&reader);
			printf("\nTimerISR\n");
			TIMER_SOFTWARE_stop_timer(timer1);
			TIMER_SOFTWARE_clear_interrupt(timer1);
		}
	}
	return 0;
}

