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

void setRegion_NA2(void)
{
	TMR_Status status;
	TMR_Region region = TMR_REGION_NA2;
	status = TMR_paramSet(&reader, TMR_PARAM_REGION_ID, &region);
	return;
}
	TMR_ReadPlan readPlan;
void setReadPlan_Simple(void)
{

	TMR_Status status;
	uint8_t antennaList[] = {(uint8_t)1U};
	uint8_t antennaCount = 1;
	TMR_TagProtocol protocol = TMR_TAG_PROTOCOL_GEN2;
	TMR_RP_init_simple(&readPlan, antennaCount, antennaList,protocol, 100U);
	status = TMR_paramSet(&reader, TMR_PARAM_READ_PLAN, &readPlan);
	return;
}

void setCompleteMetadataFlag(void)
{
	TMR_Status status;
	TMR_TRD_MetadataFlag metadata = (uint16_t)(TMR_TRD_METADATA_FLAG_ALL & (~TMR_TRD_METADATA_FLAG_TAGTYPE));
	status = TMR_paramSet(&reader, TMR_PARAM_METADATAFLAG, &metadata);
	return;
}

void Module_Reset()
{
	IO0CLR = 1 << 23;
	TIMER_SOFTWARE_Wait(1000);
	IO0SET = 1 << 23;
	TIMER_SOFTWARE_Wait(1000);
}

int main(void)
{	 
	timer_software_handler_t timer1;
	//int n = 0;
	IO0DIR |= 1 << 23;
	IO0CLR = 1 << 23;
	
	TIMER_SOFTWARE_init();
	TIMER_Init(); /*This is the HW timer*/
	UART0_Init(); /*Debugging port*/
	UART1_Init();
	printf("Main");

	InitInterrupt();
		Module_Reset();
	/*------------------------------------*/
	(void)TMR_SR_SerialTransportDummyInit(&reader.u.serialReader.transport, NULL, NULL);
	//TMR_SR_SerialReader_init(&reader);
	TMR_create(&reader, "eapi:///UART1");
	TMR_connect(&reader);
	
	printf("\nRegion to NA\n");
	setRegion_NA2();
	printf("\nReadPlan and Antenna\n");
	setReadPlan_Simple();
	/*------------------------------------*/
	//uint8_t array[24] = {0xFF, 0x13, 0x2F, 0x00, 0x00, 0x01, 0x22, 0x00, 0x00, 0x05, 0x0A, 0x22, 0x88, 0x10, 0x04, 0x1B, 0x00, 0xFA, 0x00, 0x00, 0x0F, 0xFF, 0x29, 0x0C};

 bool readFilter = false;
       TMR_paramSet(&reader, TMR_PARAM_TAGREADDATA_ENABLEREADFILTER, &readFilter);


	timer1 = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer1, MODE_0, 5000, 1);
	TIMER_SOFTWARE_reset_timer(timer1);
	printf("---start command--\n");
	TMR_startReading(&reader);
	/*------------------------------------*/
		//printf ("%d\n", n++);
		//UART1_sendbuffer(array, 24, 0);
		//TIMER_SOFTWARE_Wait(1000);
//	while(1)
//	{
//		TIMER_SOFTWARE_Wait(1000);
//		printf ("aaaa\n");
//	}
	TIMER_SOFTWARE_start_timer(timer1);
	while(1)
	{
		/*TIMER_SOFTWARE_Wait(1000);
		if(!RingBufEmpty(&uart1_ringbuff_rx))
		{
			while(!RingBufEmpty(&uart1_ringbuff_rx))
				printf("%02X ", RingBufReadOne(&uart1_ringbuff_rx));
		}*/
		//TIMER_SOFTWARE_Wait(1000);
		
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
		if (TIMER_SOFTWARE_interrupt_pending(timer1))
		{
			printf("---stop command--");
			//TMR_stopReading(&reader);
			TIMER_SOFTWARE_stop_timer(timer1);
			TIMER_SOFTWARE_clear_interrupt(timer1);
		}
	}
	return 0;
}

