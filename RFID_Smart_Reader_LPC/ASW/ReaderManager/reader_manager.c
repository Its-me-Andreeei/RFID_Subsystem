#include <LPC22xx.h>
#include <stdio.h>
#include <stdbool.h>

#include "reader_manager.h"
#include "../../utils/timer_software.h"
#include "../../mercuryapi-1.37.1.44/c/src/api/tm_reader.h"
#include "../../hal/uart1.h"
#include "../LP_Mode_manager/LP_Mode_Manager.h"


#ifndef NULL
#define NULL ((void*) 0)
#endif

static TMR_Reader reader;
static TMR_TagReadData data;
static timer_software_handler_t timer_reader_temperature; 
static timer_software_handler_t timer_route_status;
static route_status_t route_status = ON_THE_ROUTE;
static bool read_request = false;


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
	Function: 		validate_tag_criteria
	Description:	This funciton will validate if a tag is within the route or not. Will be stubed for now
	Parameters: 	EPC read from tag and it's length
	Return value:	true if tag is part of the route, false otherwise
							
*************************************************************************************************************************************************/
static bool validate_tag_criteria(const uint8_t *epc, const uint8_t epc_length)
{
	bool result = false;
	
	/*will be replaced later on*/
	if(epc[epc_length-1] == (uint8_t)0xBF)
	{
		result = true;
	}
	
	return result;
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
	
	/*Timer is used to switch to temperature check mode after a given timeout*/
	timer_reader_temperature = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer_reader_temperature, MODE_0, TEMPERATURE_CHECK_TIMEOUT, 1);
	TIMER_SOFTWARE_reset_timer(timer_reader_temperature);
	
	/*Timer is used to detect one single tag, in one iteration of state machine*/
	timer_route_status = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer_route_status, MODE_0, STATUS_NO_ON_ROUTE_TIMEOUT, 1);
	TIMER_SOFTWARE_reset_timer(timer_route_status);
}


/*************************************************************************************************************************************************
	Function: 		Reader_HW_Reset
	Description:	This function will perform a HW reset of reader module
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void Reader_HW_Reset(void)
{
	IO0CLR = (uint8_t)1 << RESET_PIN_READER_U8;
	TIMER_SOFTWARE_Wait(1000);
	IO0SET = (uint8_t)1 << RESET_PIN_READER_U8;
	TIMER_SOFTWARE_Wait(1000);
}

void Reader_Set_Read_Request(bool value)
{
	read_request = value;
}
/*************************************************************************************************************************************************
	Function: 		reader_recovery
	Description:	This function will perform recovery procedure: Will perform a HW reset of reader module, and send 4 PINGs. 
								If no none of pings will receive answear, the requence is redone up to a maximum of 4 times
	Parameters: 	void
	Return value:	TRUE = Communication is recovered
								FALSE = Permament failure state - Communication was not recovered after 4 complete sequences. No ping responses
*************************************************************************************************************************************************/
static bool_t reader_recovery()
{
	uint8_t recovery_sequence_counter = NUMBER_OF_RECOVERY_SEQUENCE_STEPS_U8;
	uint8_t result = FALSE;
	UART_TX_RX_Status_en ping_result = RETURN_NOK;
	uint8_t index;
	uint8_t number_of_correct_pings = (uint8_t)0x00;
	
	#ifdef UART1_DBG
	printf("---------chip recovery----\n");
	#endif /*UART1_DBG*/
	
	while((recovery_sequence_counter > (uint8_t)0U) && (result == FALSE))
	{
		/*Reset number of correct ping results after each retry*/
		number_of_correct_pings = (uint8_t) 0x00;
		
		/*Perform HW reset of Reader circuit*/
		Reader_HW_Reset();
		
		/*Check for several correct PINGS to see if communication is now working*/
		for(index = (uint8_t)0x00; index < NUMBER_OF_PING_CHECKS_AFTER_RESET_U8; index++)
		{
			TIMER_SOFTWARE_Wait(500);
			/*Send PING signal to Reader*/
			ping_result = UART1_send_reveice_PING();
			
			/*Check if response to PING signal is what was expected*/
			if(RETURN_OK == ping_result)
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
		
		/*Guard sequence counter in order to not reach underflow*/
		if(recovery_sequence_counter > 0)
		{
			recovery_sequence_counter --;
		}
	}
	#ifdef UART1_DBG
	printf("NUM_SEQ= %d\n", recovery_sequence_counter);
	printf("---end chip recovery----\n");
	#endif /*UART1_DBG*/
	return result; 
}

/*************************************************************************************************************************************************
	Function: 		Reader_Manager
	Description:	This manager handles the RFID reader, as a state machine. Must be called cyclically. Must not be called before ReaderInit() function
	Parameters: 	void
	Return value:	void
							
*************************************************************************************************************************************************/
void Reader_Manager(void)
{
	typedef enum ReaderManager_state_en{
		MODULE_INIT,
		CHECK_FOR_REQUEST,
		START_READING,
		STOP_READING,
		GET_TAGS, /*This will be the state will be most of time*/
		CHECK_TEMPERATURE,
		PERMANENT_FAILURE
	}ReaderManager_state_en;
	
	typedef enum StopReason_t
	{
		NO_STOP_REASON,
		STOP_FOR_TEMPERATURE,
		STOP_FOR_REQUEST_DONE,
		STOP_FOR_SLEEP
	}StopReason_t;
	
	static ReaderManager_state_en current_state_en = CHECK_FOR_REQUEST;
	int8_t temperature = (int8_t)0x00U;
	static StopReason_t request_stop_reading_reason = NO_STOP_REASON; /*This flag is used to decide if after 'stop read' should be checked for temperature, if flag = 0x01*/
	static TMR_SR_PowerMode powerManagementRequest = TMR_SR_POWER_MODE_FULL;
	
	switch(current_state_en)
	{
		/*We assume init is already done, so first state will be START_READING. We only get here by other internal states request*/
		case MODULE_INIT:
			ConfigInit();
			current_state_en = START_READING;
			break;
		
		case CHECK_FOR_REQUEST:
			
			if(true == read_request)
			{
				/*Set stay awake flag*/
				LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, true);
				
				/*A request to start reading was done, so go to START READING state next time*/
				current_state_en = START_READING;
				
				/*Reset request flag after setting next state*/
				read_request = false;
			}
			else
			{
				LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, false);
				/*Wait for new request*/
			}
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
		
			switch(request_stop_reading_reason)
			{
				case STOP_FOR_TEMPERATURE:
					current_state_en = CHECK_TEMPERATURE;
					break;
				
				case NO_STOP_REASON:
					/*In case Stop state was chosen, but no start reason setted, go back to start read*/
					current_state_en = START_READING;
					break;	
				
				case STOP_FOR_SLEEP:
						/*Stop temperature timer. Will be resume on next request and time will keep flowing after a new request will be in place*/
						TIMER_SOFTWARE_stop_timer(timer_reader_temperature);
				
					/*Remove Stay Awake flag*/
					LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, false);
					
					/*Go to Check for Request state*/
					current_state_en = CHECK_FOR_REQUEST;
					break;
					
				
				default:
					/*In case Stop state was chosen, but no valid start reason setted, go back to start read*/
					current_state_en = START_READING;
					break;
			}
			
			/*Reset flag after deciding next state*/
			request_stop_reading_reason = NO_STOP_REASON; 
			break;
		
		case GET_TAGS:
			/*Check if it's first reading iteration and timer has to be started*/
			if(!TIMER_SOFTWARE_interrupt_pending(timer_reader_temperature) && !TIMER_SOFTWARE_is_Running(timer_reader_temperature))
			{
				TIMER_SOFTWARE_reset_timer(timer_reader_temperature);
				TIMER_SOFTWARE_start_timer(timer_reader_temperature);
			}
			
			/*Check if it's first reading iteration and timer has to be started*/
			if(!TIMER_SOFTWARE_interrupt_pending(timer_route_status) && !TIMER_SOFTWARE_is_Running(timer_route_status))
			{
				TIMER_SOFTWARE_reset_timer(timer_route_status);
				TIMER_SOFTWARE_start_timer(timer_route_status);
			}
			
			/*Check if there are tags available*/
			if(TMR_SUCCESS == TMR_hasMoreTags(&reader))
			{
				TMR_getNextTag(&reader, &data);
				
				#ifdef PRINT_EPC_DBG
				printf("EPC:" );
				for(uint16_t i = 0; i< data.tag.epcByteCount; i++)
				{
					printf("%02X ", data.tag.epc[i]);
				}
				printf("\n");
				#endif /*PRINT_EPC_DBG*/
				
				if(true == validate_tag_criteria(data.tag.epc, data.tag.epcByteCount))
				{
					/*Set flag regarding "on route" status*/
					route_status = ON_THE_ROUTE;
					
					/*Reset timer if already got positive response*/
					(void)TIMER_SOFTWARE_stop_timer(timer_route_status);
					TIMER_SOFTWARE_clear_interrupt(timer_route_status);
					TIMER_SOFTWARE_reset_timer(timer_route_status);
					
					/*Select Stop Reading State for entering Sleep*/
					request_stop_reading_reason = STOP_FOR_SLEEP;
					
					/*Mark next state as Stop Reading as request is finalized*/
					current_state_en = STOP_READING;
				}
			}		
			else if(TIMER_SOFTWARE_interrupt_pending(timer_route_status))
			{
				TIMER_SOFTWARE_clear_interrupt(timer_route_status);
				TIMER_SOFTWARE_reset_timer(timer_route_status);
				
				/*After enough missings, declare not on route status*/
				route_status = NOT_ON_ROUTE;
				
				/*Select Stop Reading State for entering Sleep*/
				request_stop_reading_reason = STOP_FOR_SLEEP;
					
				/*Mark next state as Stop Reading as request is finalized*/
				current_state_en = STOP_READING;
		
			}/*After a fixed amount of ms has elaspsed, check for internal temperature of reader for safety*/
			else if(TIMER_SOFTWARE_interrupt_pending(timer_reader_temperature))
			{
				TIMER_SOFTWARE_clear_interrupt(timer_reader_temperature);
				
				(void)TIMER_SOFTWARE_stop_timer(timer_route_status);
				TIMER_SOFTWARE_reset_timer(timer_route_status);
				
				/*Request to manager to go to check temperature only after RF emissions are off. Might add An enum if more cases will be possible from STOP state*/
				request_stop_reading_reason = STOP_FOR_TEMPERATURE;
				
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
			#ifdef UART1_DBG
			printf("---\n");
			printf("%d\n", temperature);
			printf("---\n");
			#endif /*UART1_DBG*/
			
			
			if(temperature < 45U)
			{
				/*If temperature is acceptable but we are in LP due to a previous overtemperature*/
				if(TMR_SR_POWER_MODE_FULL != powerManagementRequest)
				{
					/*Sett reader back to High Power*/
					powerManagementRequest = TMR_SR_POWER_MODE_FULL;
					(void)TMR_paramSet(&reader, TMR_PARAM_POWERMODE, &powerManagementRequest);
				}
				/*We can keep reading as temperature is OK*/
				current_state_en = START_READING;
			}
			if(temperature >=45U && temperature < 55U) /*thermal warning*/
			{
				
				if(TMR_SR_POWER_MODE_MEDSAVE != powerManagementRequest)
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
			/*Do not set anymore Stay awake reason flag*/
			LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, false);
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
		if(UART1_get_CommErrors() > 0U) 
		{
			TIMER_SOFTWARE_Wait(500);
			(void)TMR_stopReading(&reader);
			
			#ifdef UART1_DBG
			printf("COMM ERROR: ");
			printf("%d\n", UART1_get_CommErrors());
			#endif /*UART1_DBG*/
			
			/*Check if chip recovery is successfull*/
			if((uint8_t)0x01U == reader_recovery())
			{
				current_state_en = MODULE_INIT;
				
				#ifdef UART1_DBG
				printf("RECOVERED\n");
				#endif /*UART1_DBG*/
			}
			else
			{
				
				current_state_en = PERMANENT_FAILURE;
				
				#ifdef UART1_DBG
				printf("PERMANENT_FAILURE\n");
				#endif /*UART1_DBG*/
			}		
		}
	}
}

route_status_t Reader_GET_route_status(void)
{
	route_status_t result;
	result = route_status;
	route_status = ROUTE_PENDING;
	return result;
}

