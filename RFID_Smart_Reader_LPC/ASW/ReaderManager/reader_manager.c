#include "reader_manager.h"
#include <LPC22xx.h>
#include "../../utils/timer_software.h"
#include "../../mercuryapi-1.37.1.44/c/src/api/tm_reader.h"
#include <stdio.h>
#include "../../hal/uart1.h"

#ifndef NULL
#define NULL ((void*) 0)
#endif

static TMR_Reader reader;
static TMR_TagReadData data;
static timer_software_handler_t timer_reader; 

typedef enum ReaderManager_state_en{
	MODULE_INIT,
	START_READING,
	STOP_READING,
	GET_TAGS, /*This will be the state will be most of time*/
	CHECK_TEMPERATURE,
	PERMANENT_FAILURE
}ReaderManager_state_en;

typedef enum Recovery_sequence_step_en
{
	APPLY_PING,
	CHECK_RX_EMPTY,
	CHECK_RESPONSE,
	PERFORM_SW_RESET,
	PERFORM_HW_RESET,
	END_OF_SEQUENCE
}Recovery_sequence_step_en;

/*This function is not recreating the timer element and reader struct*/
/*************************************************************************************************************************************************
	Function: 		ConfigInit
	Description:	This function must be called only after reader is created. Will check the communication with RFID Reader, and set the configuration inside Reader system trough UART1
								This function is not recreating the timer element and reader struct
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
static void ConfigInit(void)
{
	const TMR_Region region = TMR_REGION_NA2;
	TMR_ReadPlan readPlan;
	uint8_t antennaList[] = {(uint8_t)1U};
	const uint8_t antennaCount = 1;
	const TMR_TagProtocol protocol = TMR_TAG_PROTOCOL_GEN2;
	TMR_TRD_MetadataFlag metadata = (uint16_t)(TMR_TRD_METADATA_FLAG_ALL & (~TMR_TRD_METADATA_FLAG_TAGTYPE));
	bool readFilter = false;
	uint16_t asyncOnTime= (uint16_t) 500; /*Time in ms*/
	uint16_t asyncOffTime= (uint16_t) 1000; /*Time in ms*/
	
	(void)TMR_connect(&reader);
	
	(void)TMR_paramSet(&reader, TMR_PARAM_REGION_ID, &region);
	
	(void)TMR_RP_init_simple(&readPlan, antennaCount, antennaList,protocol, 100U);
	(void)TMR_paramSet(&reader, TMR_PARAM_READ_PLAN, &readPlan);
	
	(void)TMR_paramSet(&reader, TMR_PARAM_METADATAFLAG, &metadata);
	
	(void)TMR_paramSet(&reader, TMR_PARAM_TAGREADDATA_ENABLEREADFILTER, &readFilter);
	
	(void)TMR_paramSet(&reader, TMR_PARAM_READ_ASYNCONTIME, &asyncOnTime);
	
	(void)TMR_paramSet(&reader, TMR_PARAM_READ_ASYNCOFFTIME, &asyncOffTime);
}

/*************************************************************************************************************************************************
	Function: 		ReaderManagerInit
	Description:	This funciton will initialize the reader manager, and physical reader and required internal software timer
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void ReaderManagerInit(void)
{
	(void)TMR_SR_SerialTransportDummyInit(&reader.u.serialReader.transport, NULL, NULL);
	
	(void)TMR_create(&reader, "eapi:///UART1");

	ConfigInit();
	
	timer_reader = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer_reader, MODE_0, 10000, 1);
	TIMER_SOFTWARE_reset_timer(timer_reader);
}


/*************************************************************************************************************************************************
	Function: 		Reader_HW_Reset
	Description:	This function will perform a HW reset of reader module
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void Reader_HW_Reset(void)
{
	IO0CLR = (uint8_t)1 << (uint8_t)23;
	TIMER_SOFTWARE_Wait(1000);
	IO0SET = (uint8_t)1 << (uint8_t)23;
	TIMER_SOFTWARE_Wait(1000);
}

/*************************************************************************************************************************************************
	Function: 		reader_recovery
	Description:	This function will perform recovery procedure: Will perform a HW reset of reader module, and send 4 PINGs. 
								If no none of pings will receive answear, the requence is redone up to a maximum of 4 times
	Parameters: 	void
	Return value:	TRUE = Communication is recovered
								FALSE = Permament failure state - Communication was not recovered after 4 complete sequences. No ping responses
*************************************************************************************************************************************************/
static uint8_t reader_recovery()
{
	#define NUMBER_OF_PING_CHECKS_AFTER_RESET ((uint8_t)4)
	#define NUMBER_OF_RECOVERY_SEQUENCE_STEPS ((uint8_t)0x03U)
	
	uint8_t recovery_sequence_counter = NUMBER_OF_RECOVERY_SEQUENCE_STEPS;
	uint8_t result = FALSE;
	uint8_t ping_result = FALSE;
	uint8_t index;
	uint8_t number_of_correct_pings = (uint8_t)0x00;
	
	printf("---------chip recovery----\n");
	while((recovery_sequence_counter > (uint8_t)0U) && (result == FALSE))
	{
		/*Reset number of correct ping results after each retry*/
		number_of_correct_pings = (uint8_t) 0x00;
		
		/*Perform HW reset of Reader circuit*/
		Reader_HW_Reset();
		
		/*Check for several correct PINGS to see if communication is now working*/
		for(index = (uint8_t)0x00; index < NUMBER_OF_PING_CHECKS_AFTER_RESET; index++)
		{
			TIMER_SOFTWARE_Wait(500);
			/*Send PING signal to Reader*/
			ping_result = UART1_send_reveice_PING();
			
			/*Check if response to PING signal is not what was expected*/
			if(TRUE == ping_result)
			{
				number_of_correct_pings++;
				break; /*Do not send anymore pings, we got a correct one*/
			}
		}
		/*If there are correct responses to ping, we can end the sequence*/
		if((uint8_t)0x00 != number_of_correct_pings)
		{
			result = TRUE;
			break;
		}
		else
		{
			/*This assignment is added for robustness. 'result' is initially set to FALSE anyways*/
			result = FALSE;
		}
		if(recovery_sequence_counter > 0)
		{
			recovery_sequence_counter --;
		}
	}
	printf("NUM_SEQ= %d\n", recovery_sequence_counter);
	printf("---end chip recovery----\n");
	return result; 
}

/*************************************************************************************************************************************************
	Function: 		ReaderManagerInit
	Description:	This manager handles the RFID reader, as a state machine. Must be called cyclically. Must not be called before ReaderInit() function
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void Reader_Manager(void)
{
	static ReaderManager_state_en current_state_en = START_READING;
	int8_t temperature = (int8_t)0x00U;
	static uint8_t request_temp_check_after_stop= (uint8_t)0x00U; /*This flag is used to decide if after 'stop read' should be checked for temperature, if flag = 0x01*/
	static TMR_SR_PowerMode powerManagementRequest = TMR_SR_POWER_MODE_FULL;
	
	switch(current_state_en)
	{
		/*We assume init is already done. We only get here by other internal states request*/
		case MODULE_INIT:
			ConfigInit();
			current_state_en = START_READING;
			break;
		
		case START_READING:
			(void)TMR_startReading(&reader);
		
			current_state_en = GET_TAGS;
			break;
		
		case STOP_READING:
			/*This will stop RF emissions*/
			(void)TMR_stopReading(&reader);

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
				TIMER_SOFTWARE_reset_timer(timer_reader);
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

			(void)TMR_paramGet(&reader, TMR_PARAM_RADIO_TEMPERATURE, &temperature);

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
					(void)TMR_paramSet(&reader, TMR_PARAM_POWERMODE, &powerManagementRequest);
				}
				/*We can keep reading as temperature is OK*/
				current_state_en = START_READING;
			}
			if(temperature >=45 && temperature < 55) /*thermal warning*/
			{
				
				if(powerManagementRequest != TMR_SR_POWER_MODE_MEDSAVE)
				{
					powerManagementRequest = TMR_SR_POWER_MODE_MEDSAVE;
					(void)TMR_paramSet(&reader, TMR_PARAM_POWERMODE, &powerManagementRequest);
				}
				
				/*We can keep reading as temperature is OK*/
				current_state_en = START_READING;
			}
			else if(temperature >= (uint8_t)55U) /*Check if maximal operational temperature is almost reached*/
			{
				/*Set reader to Low Power until temperature is decreased*/
				powerManagementRequest = TMR_SR_POWER_MODE_MAXSAVE;
				
				(void)TMR_paramSet(&reader, TMR_PARAM_POWERMODE, &powerManagementRequest);
				
				/*Manager will be stucked in this state untill temperature decreases*/
				current_state_en = CHECK_TEMPERATURE;
				/*Delay between checks in order to not increase payload on UART*/
				TIMER_SOFTWARE_Wait(500);
			}
			
			break;
			
		case PERMANENT_FAILURE:
			/*Do nothing for now*/
			break;
		
		default:
			/*Invalid state reached, go back to 'GET TAGS' state*/
			current_state_en = GET_TAGS;
			break;
	}

	/*Do not perform chip recovery if already in permanent failure mode*/
	if(current_state_en != PERMANENT_FAILURE)
	{
	
		/*Check for communication errors*/
		if(UART1_get_CommErrors() > (uint8_t)0U)
		{ 
			TIMER_SOFTWARE_Wait(500);
			(void)TMR_stopReading(&reader);
			printf("COMM ERROR: ");
			printf("%d\n", UART1_get_CommErrors());
			
			/*Check if chip recovery is successfull*/
			if((uint8_t)0x01U == reader_recovery())
			{
				current_state_en = MODULE_INIT;
				printf("RECOVERED\n");
			}
			else
			{
				
				current_state_en = PERMANENT_FAILURE;
				printf("PERMANENT_FAILURE\n");
			}		
		}
	}
}
