#include "reader_manager.h"
#include <LPC22xx.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "../../utils/timer_software.h"
#include "../../mercuryapi-1.37.1.44/c/src/api/tm_reader.h"

#include "../../hal/uart1.h"
#include "../LP_Mode_Manager/LP_Mode_Manager.h"
#include "../WiFiManager/wifi_manager.h"
#include "../../PlatformTypes.h"

#ifndef NULL
#define NULL ((void*) 0)
#endif

typedef enum failure_reason_t
{
	NO_FAILURE_PRESENT,
	INTERNAL_FAILURE,
	WI_FI_MODULE_NOT_PRESENT
}failure_reason_t;

static TMR_Reader reader;
static TMR_TagReadData data;
static timer_software_handler_t timer_route_status_and_panic;
static ReaderRequest_t reader_request = NO_REQUEST;
static volatile failure_reason_t failure_reason = NO_FAILURE_PRESENT;
static state_t WIFI_server_data_ready = STATE_PENDING;

static u8 wifi_buffer_response[100];
static u8 wifi_buffer_length;


static void disable_HW_Reader(void)
{
	IO0CLR = (uint8_t)1 << RESET_PIN_READER_U8;
}

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
	uint16_t asyncOnTime= (uint16_t) 1000; /*Time in ms*/
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
	Function: 		send_validate_tag_criteria_cmd
	Description:	This funciton will validate if a tag is within the route or not. Will be stubed for now
	Parameters: 	EPC read from tag and it's length
	Return value:	true if tag is part of the route, false otherwise
							
*************************************************************************************************************************************************/
static state_t send_validate_tag_criteria_cmd(const uint8_t *epc, const uint8_t epc_length)
{
	state_t result = STATE_PENDING;
	bool bool_result;
	if((epc == NULL) || (epc_length <= (uint8_t)0))
	{
		result = STATE_NOK;
	}
	else
	{
		
		bool_result = Wifi_SET_Command_Request(WIFI_COMMAND_CHECK_TAG, epc, epc_length);
		/*Means wifi_manager is busy right now*/
		if(false == bool_result)
		{
			result = STATE_PENDING;
		}
		else if(true == bool_result)
		{
			result = STATE_OK;
		}
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
	
	timer_route_status_and_panic = TIMER_SOFTWARE_request_timer();
	TIMER_SOFTWARE_configure_timer(timer_route_status_and_panic, MODE_0, STATUS_NO_ON_ROUTE_TIMEOUT, 1);
	TIMER_SOFTWARE_reset_timer(timer_route_status_and_panic);
	
	LP_Set_InitFlag(FUNC_RFID_READER_MANAGER, true);
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

ReaderRequest_t Reader_GET_request_status(void)
{
	ReaderRequest_t result;
	result = reader_request;
	
	if(REQUEST_FINISHED == reader_request)
	{
		reader_request = NO_REQUEST;
	}
	return result;
}

bool Reader_GET_TagInformation(u8 *data, u8 *len)
{
	bool result;
	if((data == NULL) || (len == NULL))
	{
		result = false;
	}
	else
	{
		if(STATE_OK == WIFI_server_data_ready)
		{
			memcpy(data, wifi_buffer_response, wifi_buffer_length);
			*len = wifi_buffer_length;
			
			/*Reset this flag as an interface handsake*/
			WIFI_server_data_ready = STATE_PENDING;
			result = true;
		}
		else
		{
			result = false;
		}
	}
	return result;
}

bool Reader_SET_read_request(bool request)
{
	bool result = false;
	
	if(NO_REQUEST == reader_request)
	{
		if(true == request)
		{
			reader_request = READ_REQUEST_ASKED;
			
		}
		else
		{
			reader_request = NO_REQUEST;
		}
		result = true;
	}
	
	return result;
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
	#define ROUTE_STATUS_BIT_POS_NUM_U8 ((u8)1U)
	
	typedef enum ReaderManager_state_en{
		CHECK_FOR_REQUESTS,
		MODULE_INIT,
		START_READING,
		STOP_READING,
		GET_TAGS, /*This will be the state will be most of time*/
		SEND_CHECK_TAG_CMD,
		RECEIVE_CHECK_TAG_RESPONSE,
		CHECK_TEMPERATURE,
		PERMANENT_FAILURE,
		PANIC_STATE
	}ReaderManager_state_en;
	
	typedef enum StopReason_t {
		NO_STOP_REASON,
		STOP_FOR_TEMPERATURE,
		STOP_FOR_SLEEP
	}StopReason_t;
	
	static ReaderManager_state_en current_state_en = CHECK_FOR_REQUESTS;
	int8_t temperature = (int8_t)0x00U;
	static StopReason_t request_stop = NO_STOP_REASON; /*This flag is used to decide if after 'stop read' should be checked for temperature, if flag = 0x01*/
	static TMR_SR_PowerMode powerManagementRequest = TMR_SR_POWER_MODE_FULL;
	static bool panic_sequence_in_progress = false;
	state_t result_state;
	
	if((false == LP_Get_Functionality_Init_State(FUNC_WIFI_MANAGER)) && (false == Wifi_GET_is_Wifi_Connected_status()) && (false == panic_sequence_in_progress) && (NO_FAILURE_PRESENT == failure_reason))
	{
		printf("READER PANIC ENTERED\n");
		/*Mark Panic sequence as 'in progress'*/
		panic_sequence_in_progress = true;
		/*If WI-FI module is not INIT, wait some time and might get init. Maybe is in it's recovery state or lost connection to internet*/
		TIMER_SOFTWARE_stop_timer(timer_route_status_and_panic);
		TIMER_SOFTWARE_reset_timer(timer_route_status_and_panic);
		TIMER_SOFTWARE_start_timer(timer_route_status_and_panic);
		
		
		LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, true);
		
		/*Stop timer if RF emissions are in progress*/
		if((START_READING == current_state_en) || (GET_TAGS == current_state_en) || (STOP_READING == current_state_en))
		{
			TMR_stopReading(&reader);
			TMR_flush(&reader);
		}
		current_state_en = PANIC_STATE;
	}
	else
	{
		/*No state change if WI FI module is still present*/
	}
	
	switch(current_state_en)
	{
		/*We assume init is already done, so first state will be START_READING. We only get here by other internal states request*/
		case MODULE_INIT:
			ConfigInit();
			LP_Set_InitFlag(FUNC_RFID_READER_MANAGER, true);
			LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, false);
		
			/*Reader is ready to process new requests. May arive here after error recoveries*/
			reader_request = NO_REQUEST;
		
			current_state_en = CHECK_FOR_REQUESTS;
			break;
		
		case CHECK_FOR_REQUESTS:
			if(READ_REQUEST_ASKED == reader_request)
			{
				LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, true);
				current_state_en = START_READING;
				reader_request = REQUEST_IN_PROGRESS;
			}
			else
			{
				current_state_en = CHECK_FOR_REQUESTS;
				LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, false);
			}
			break;
			
		case PANIC_STATE:
			LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, true);
			if(true == TIMER_SOFTWARE_is_Running(timer_route_status_and_panic))
			{
				if(true == LP_Get_Functionality_Init_State(FUNC_WIFI_MANAGER))
				{
					TIMER_SOFTWARE_stop_timer(timer_route_status_and_panic);
					TIMER_SOFTWARE_reset_timer(timer_route_status_and_panic);
					failure_reason = NO_FAILURE_PRESENT;
					current_state_en = CHECK_FOR_REQUESTS;
				}
				else
				{
					/*Wi fi Module still absent*/
					current_state_en = PANIC_STATE;
				}
			}
			else
			{
				if(true == TIMER_SOFTWARE_interrupt_pending(timer_route_status_and_panic))
				{
					/*Reset timer*/
					TIMER_SOFTWARE_clear_interrupt(timer_route_status_and_panic);
					TIMER_SOFTWARE_reset_timer(timer_route_status_and_panic);
					failure_reason = WI_FI_MODULE_NOT_PRESENT;
					current_state_en = PERMANENT_FAILURE;
				}
			}
			break;
		
		case START_READING:
			(void)TMR_startReading(&reader);
			TIMER_SOFTWARE_reset_timer(timer_route_status_and_panic);
			TIMER_SOFTWARE_start_timer(timer_route_status_and_panic);
			current_state_en = GET_TAGS;
			break;
		
		case STOP_READING:
			/*This will stop RF emissions*/
			(void)TMR_stopReading(&reader);

			/* flush of SW buffer is done because stop reading function is not 'receiving' the RX buffer thus response is 
			 * stucked and will interfere with temperature result*/
			(void)TMR_flush(&reader); 
		
			
			switch(request_stop){
				case STOP_FOR_TEMPERATURE:
					current_state_en = CHECK_TEMPERATURE;
				break;
				
				case STOP_FOR_SLEEP:
					/*Do nothing now*/
				break;
				
				default:
					current_state_en = GET_TAGS;
				break;
			}
			/*Reset flag after processing request*/
			request_stop = NO_STOP_REASON;
			break;
		
		case GET_TAGS:
			
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
				current_state_en = SEND_CHECK_TAG_CMD;
			}
			
			if(TIMER_SOFTWARE_interrupt_pending(timer_route_status_and_panic))
			{
				TIMER_SOFTWARE_clear_interrupt(timer_route_status_and_panic);
				TIMER_SOFTWARE_reset_timer(timer_route_status_and_panic);
				
				/*After timeout reached, finish requst*/
				reader_request = REQUEST_FINISHED;
				
				/*Request to manager to go to check temperature only after RF emissions are off. Might add An enum if more cases will be possible from STOP state*/
				request_stop = STOP_FOR_TEMPERATURE;
				
				current_state_en = STOP_READING;
			}
			else /*There are not yet elaspsed 5 seconds*/
			{
				//current_state_en = SEND_CHECK_TAG_CMD;
			}
			break;
			
		case SEND_CHECK_TAG_CMD:
			result_state = send_validate_tag_criteria_cmd(data.tag.epc, data.tag.epcByteCount);
			if(STATE_OK == result_state)
			{
				current_state_en = RECEIVE_CHECK_TAG_RESPONSE;
			}
			else if(STATE_PENDING == result_state)
			{
				if(TIMER_SOFTWARE_interrupt_pending(timer_route_status_and_panic))
				{
					request_stop = STOP_FOR_TEMPERATURE;
					/*After enough missings, declare request finished*/
					reader_request = REQUEST_FINISHED;
					
					current_state_en = STOP_READING;
				}
				else
				{
					current_state_en = SEND_CHECK_TAG_CMD;
				}
			}
			else
			{
				current_state_en = GET_TAGS;
			}
			break;
		
		case RECEIVE_CHECK_TAG_RESPONSE:
			/*If statuses have been read , we can get to Reader Manager the updated status -> will not loose information*/
			if(STATE_PENDING == WIFI_server_data_ready)
			{
				WIFI_server_data_ready = Wifi_GET_passtrough_response(wifi_buffer_response, &wifi_buffer_length);
			}
			if(STATE_OK == WIFI_server_data_ready)
			{
				TIMER_SOFTWARE_stop_timer(timer_route_status_and_panic);
				TIMER_SOFTWARE_clear_interrupt(timer_route_status_and_panic);
				TIMER_SOFTWARE_reset_timer(timer_route_status_and_panic);
				
				request_stop = STOP_FOR_TEMPERATURE;
				reader_request = REQUEST_FINISHED;
				current_state_en = STOP_READING;
			}
			else
			{
				/*If request was not accepted, try next time*/
				if(TIMER_SOFTWARE_interrupt_pending(timer_route_status_and_panic))
				{
					TIMER_SOFTWARE_clear_interrupt(timer_route_status_and_panic);
					TIMER_SOFTWARE_reset_timer(timer_route_status_and_panic);
					
					request_stop = STOP_FOR_TEMPERATURE;

					reader_request = REQUEST_FINISHED;
					
					current_state_en = STOP_READING;
				}
				else
				{
					/*If request was not accepted, try next time*/
					current_state_en = RECEIVE_CHECK_TAG_RESPONSE;
				}
			}
			break;
		
		case CHECK_TEMPERATURE:
			/*Reading temporary stopped by now for temperature measurement*/
			/*Wait 500ms in order to not receive remianing echos from when reading was active*/
			TIMER_SOFTWARE_Wait(500);
		
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
				/*We can keep processing requests as temperature is OK*/
				current_state_en = CHECK_FOR_REQUESTS;
				
				/*Reader Manager finished current task, he can sleep now*/
				LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, false);
			}
			if(temperature >=45U && temperature < 55U) /*thermal warning*/
			{
				
				if(TMR_SR_POWER_MODE_MEDSAVE != powerManagementRequest)
				{
					powerManagementRequest = TMR_SR_POWER_MODE_MEDSAVE;
					(void)TMR_paramSet(&reader, TMR_PARAM_POWERMODE, &powerManagementRequest);
				}
				
				/*We can keep reading as temperature is OK*/
				current_state_en = CHECK_FOR_REQUESTS;
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
			/*Disconnect HW Module*/
			disable_HW_Reader();
			printf("Reader disabled\n");
			
			/*In this state the reader is not Init anymore*/
			LP_Set_InitFlag(FUNC_RFID_READER_MANAGER, false);
			
			LP_Set_StayAwake(FUNC_RFID_READER_MANAGER , false);
		
			if(WI_FI_MODULE_NOT_PRESENT == failure_reason)
			{				
				/*If Wi Fi module is not present due to internet, and if internet comes back, we can recover*/
				if(true == LP_Get_Functionality_Init_State(FUNC_WIFI_MANAGER))
				{
					/*Clear the failure*/
					failure_reason = NO_FAILURE_PRESENT;
					
					/*Perform again module init*/
					current_state_en = MODULE_INIT;
					
					/*Reset Panic sequence flag*/
					panic_sequence_in_progress = false;
					
					/*Stay awake in order to perform module init*/
					LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, true);
					
					/*Perform Reader HW reset before INIT*/
					Reader_HW_Reset();
				}
				else
				{
					/*Module still absent. Stay here*/
					current_state_en = PERMANENT_FAILURE;
				}
			}
			else
			{
				/*This is a permanent failure. Will not recover*/
			}
			/*At this point, reader is not powered anymore, and Reader manager removed for good stay awake flag*/
			
			/*Do not process any new requests. If reader will recover from any failure (such as WI FI failure) this will be reseted in INIT state*/
			reader_request = READER_IN_FAILURE;
			break;
		
		default:
			/*Invalid state reached, go back to 'CHECK_FOR_REQUESTS' state*/
			current_state_en = CHECK_FOR_REQUESTS;
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
			LP_Set_InitFlag(FUNC_RFID_READER_MANAGER, false);
			
			#ifdef UART1_DBG
			printf("COMM ERROR: ");
			printf("%d\n", UART1_get_CommErrors());
			#endif /*UART1_DBG*/
			
			/*Check if chip recovery is successfull*/
			if(true == reader_recovery())
			{
				current_state_en = MODULE_INIT;
				
				#ifdef UART1_DBG
				printf("RECOVERED\n");
				#endif /*UART1_DBG*/
			}
			else
			{
				failure_reason = INTERNAL_FAILURE;
				current_state_en = PERMANENT_FAILURE;
				
				/*Reader is in PERMANENT_FAILURE, no need to stay awake*/
				LP_Set_StayAwake(FUNC_RFID_READER_MANAGER, false);
				
				/*Disable reader as it is not anymore usable*/
				disable_HW_Reader();
				
				#ifdef UART1_DBG
				printf("PERMANENT_FAILURE\n");
				#endif /*UART1_DBG*/
			}		
		}
	}
}

bool Reader_GET_internal_failure_status(void)
{
	bool result = false;
	if(INTERNAL_FAILURE == failure_reason)
	{
		result = true;
	}
	else
	{
		result = false;
	}
	return result;
}

