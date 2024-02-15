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
static timer_software_handler_t timer_reader;
static uint8_t commErrors = 0; /*To be added in UART1 API and exposed through an interface*/

typedef enum ReaderManager_state_en{
	MODULE_INIT,
	START_READING,
	STOP_READING,
	GET_TAGS, /*This will be the state will be most of time*/
	CHECK_TEMPERATURE,
	ERROR_MANAGEMENT
}ReaderManager_state_en;

static void commErrorMonitor(TMR_Status status_to_check)
{
	if(TMR_SUCCESS != status_to_check)
	{
		commErrors ++;
	}
}

/*This function is not recreating the timer element and reader struct*/
static void ConfigInit(void)
{
	TMR_Status status = TMR_SUCCESS;
	const TMR_Region region = TMR_REGION_NA2;
	TMR_ReadPlan readPlan;
	uint8_t antennaList[] = {(uint8_t)1U};
	const uint8_t antennaCount = 1;
	const TMR_TagProtocol protocol = TMR_TAG_PROTOCOL_GEN2;
	TMR_TRD_MetadataFlag metadata = (uint16_t)(TMR_TRD_METADATA_FLAG_ALL & (~TMR_TRD_METADATA_FLAG_TAGTYPE));
	bool readFilter = false;
	uint16_t asyncOnTime= (uint16_t) 500; /*Time in ms*/
	uint16_t asyncOffTime= (uint16_t) 1000; /*Time in ms*/
	
	status = TMR_connect(&reader);
	commErrorMonitor(status);
	
	status = TMR_paramSet(&reader, TMR_PARAM_REGION_ID, &region);
	commErrorMonitor(status);
	
	(void)TMR_RP_init_simple(&readPlan, antennaCount, antennaList,protocol, 100U);
	status = TMR_paramSet(&reader, TMR_PARAM_READ_PLAN, &readPlan);
	commErrorMonitor(status);
	
	status = TMR_paramSet(&reader, TMR_PARAM_METADATAFLAG, &metadata);
	commErrorMonitor(status);
	
	status = TMR_paramSet(&reader, TMR_PARAM_TAGREADDATA_ENABLEREADFILTER, &readFilter);
	commErrorMonitor(status);
	
	status = TMR_paramSet(&reader, TMR_PARAM_READ_ASYNCONTIME, &asyncOnTime);
	commErrorMonitor(status);
	
	status = TMR_paramSet(&reader, TMR_PARAM_READ_ASYNCOFFTIME, &asyncOffTime);
	commErrorMonitor(status);
}

void ReaderManagerInit(void)
{
	(void)TMR_SR_SerialTransportDummyInit(&reader.u.serialReader.transport, NULL, NULL);
	
	(void)TMR_create(&reader, "eapi:///UART1");

	ConfigInit();
	
	timer_reader = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer_reader, MODE_0, 10000, 1);
	TIMER_SOFTWARE_reset_timer(timer_reader);
}



void Reader_HW_Reset(void)
{
	IO0CLR = (uint8_t)1 << (uint8_t)23;
	TIMER_SOFTWARE_Wait(1000);
	IO0SET = (uint8_t)1 << (uint8_t)23;
	TIMER_SOFTWARE_Wait(1000);
}

void Reader_Manager(void)
{
	static ReaderManager_state_en current_state_en = START_READING;
	int8_t temperature = (int8_t) 0U;
	static uint8_t request_temp_check_after_stop= 0x00; /*This flag is used to decide if after 'stop read' should be checked for temperature, if flag = 0x01*/
	static TMR_SR_PowerMode powerManagementRequest = TMR_SR_POWER_MODE_FULL;
	TMR_Status status = TMR_SUCCESS;
	
	switch(current_state_en)
	{
		/*We assume init is already done. We only get here by other internal states request*/
		case MODULE_INIT:
			ConfigInit();
			current_state_en = START_READING;
			break;
		
		case START_READING:
			status = TMR_startReading(&reader);
			commErrorMonitor(status);
		
			current_state_en = GET_TAGS;
			break;
		
		case STOP_READING:
			/*This will stop RF emissions*/
			status = TMR_stopReading(&reader);
			commErrorMonitor(status);
			/* flush of SW buffer is done because stop reading function is not 'receiving' the RX buffer thus response is 
			 * stucked and will interfere with temperature result*/
			(void)TMR_flush(&reader); 
		
			if(request_temp_check_after_stop == 0x01)
			{
				/*Reset flag after processing request*/
				request_temp_check_after_stop = 0x00; 
				
				current_state_en = CHECK_TEMPERATURE;
			}
			/*TBD: To be checked for else case and other strategies*/
			break;
		
		case GET_TAGS:
			/*Check if it's first reading iteration and timer has to be started*/
			if(!TIMER_SOFTWARE_interrupt_pending(timer_reader) && !TIMER_SOFTWARE_is_Running(timer_reader))
			{
				TIMER_SOFTWARE_start_timer(timer_reader);
			}
			
			/*Check if there are tags available*/
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
			
			/*After a fixed amount of ms has elaspsed, check for internal temperature of reader for safety*/
			if(TIMER_SOFTWARE_interrupt_pending(timer_reader))
			{
				TIMER_SOFTWARE_clear_interrupt(timer_reader);
				
				/*Request to manager to go to check temperature only after RF emissions are off*/
				request_temp_check_after_stop = 0x01;
				
				current_state_en = STOP_READING;
			}
			else /*There are not yet elaspsed 5 seconds*/
			{
				current_state_en = GET_TAGS;
			}
			break;
		
		case CHECK_TEMPERATURE:
			/*Reading temporary stopped by now for temperature measurement*/

			status = TMR_paramGet(&reader, TMR_PARAM_RADIO_TEMPERATURE, &temperature);
			commErrorMonitor(status);

			printf("---\n");
			printf("%d\n", temperature);
			printf("---\n");
			
			
			if(temperature < 45)
			{
				/*If temperature is acceptable but we are in LP due to a previous overtemperature*/
				if(powerManagementRequest != TMR_SR_POWER_MODE_FULL)
				{
					/*Sett reader back to High Power*/
					powerManagementRequest = TMR_SR_POWER_MODE_FULL;
					status = TMR_paramSet(&reader, TMR_PARAM_POWERMODE, &powerManagementRequest);
					commErrorMonitor(status);
				}
				/*We can keep reading as temperature is OK*/
				current_state_en = START_READING;
			}
			if(temperature >=45 && temperature < 55) /*thermal warning*/
			{
				
				if(powerManagementRequest != TMR_SR_POWER_MODE_MEDSAVE)
				{
					powerManagementRequest = TMR_SR_POWER_MODE_MEDSAVE;
					status = TMR_paramSet(&reader, TMR_PARAM_POWERMODE, &powerManagementRequest);
					commErrorMonitor(status);
				}
				
				/*We can keep reading as temperature is OK*/
				current_state_en = START_READING;
			}
			else if(temperature >= (uint8_t)55U) /*Check if maximal operational temperature is almost reached*/
			{
				/*Set reader to Low Power until temperature is decreased*/
				powerManagementRequest = TMR_SR_POWER_MODE_MAXSAVE;
				
				status = TMR_paramSet(&reader, TMR_PARAM_POWERMODE, &powerManagementRequest);
				commErrorMonitor(status);
				
				/*Manager will be stucked in this state untill temperature decreases*/
				current_state_en = CHECK_TEMPERATURE;
				/*Delay between checks in order to not increase payload on UART*/
				TIMER_SOFTWARE_Wait(500);
			}
			
			break;
		
		case ERROR_MANAGEMENT:
			/*TBD: To be added soft reset first*/
			Reader_HW_Reset();
			commErrors = 0;
			current_state_en = MODULE_INIT;
			break;
		
		default:
			/*Invalid state reached, go back to 'GET TAGS' state*/
			current_state_en = GET_TAGS;
			break;
	}

	/*TBD: To be moved in UART1 or to be removed the error manager case*/
	if(commErrors > 0)
	{
		current_state_en = ERROR_MANAGEMENT; /*Override any state as comm is lost*/
	}
}
