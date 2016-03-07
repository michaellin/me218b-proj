/****************************************************************************
 Module
   SendingCommSM.c

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "SendingCommSM.h"
#include "CommService.h"
#include "Master.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE WaitingInSend
#define StatusRequest 0xc0
#define ChangeCityRequest 0x80
#define Query 0x40
#define CommandMask 0xc0

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringSendingAllBytes( ES_Event Event);
void EOT_Response (void);
static void StoreByte(uint8_t,uint8_t);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static SendingState_t CurrentState;
static ES_Event ReturnEvent;
static ES_Event ThisEvent;
static uint8_t Byte2Send; //hold the value to the byte to be sent out

// Storage arrays for packet transfers
static uint8_t StatusArray[5];
static uint8_t ChangeCityArray[5];
static uint8_t QueryArray[5];

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    SendingCommSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.

****************************************************************************/
ES_Event RunSendingCommSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   SendingState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ReturnEvent = CurrentEvent; // assume we are not consuming event
   switch ( CurrentState )
   {
       case WaitingInSend :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
					
         //process any events
				switch (CurrentEvent.EventType)
				{
					 case ES_SendCMD : //If event is event one
							// Execute action function for state one : event one
							//Keep the byte that we are trying to send
   						Byte2Send = CurrentEvent.EventParam;
							//printf("SPI sending command 0x%x\r\n", Byte2Send);
							NextState = SendingAllBytes;//Decide what the next state will be
							// for internal transitions, skip changing MakeTransition
							//mark that we are taking a transition
							MakeTransition = true;
							// if transitioning to a state with history change kind of entry
							EntryEventKind.EventType = ES_ENTRY;
							// optionally, consume or re-map this event for the upper
							// level state machine
							ReturnEvent.EventType = ES_NO_EVENT;
							break;
				}
				break;
			case SendingAllBytes : 
				CurrentEvent = DuringSendingAllBytes(CurrentEvent);
				ES_Event ThisEvent;
				switch (CurrentEvent.EventType)
				{
					case ES_EOT:
						// We took the following post out, because we want to do it after the 2 ms timer is up (Moved to CommService)
						//ThisEvent.EventType = ES_SPIWriteDone; //Send indicator that SPI write is done
						//PostMasterSM(ThisEvent);
						ES_Timer_InitTimer(PacTimer, 2); // Wait 2 ms between transactions
						ReturnEvent.EventType = ES_NO_EVENT;
					break;
					case ES_TIMEOUT:
						if (CurrentEvent.EventParam == PacTimer) {
							//printf("Getting back to ready to send\r\n");
							NextState = WaitingInSend;
							MakeTransition = true;
							//Do not consume event so that higher level can transition as well
//							ReturnEvent.EventType = ES_NO_EVENT;
						}
					break;
				}
			break;
		}
    //   If we are making a state transition
    if (MakeTransition == true)
    {  //printf("NextState: %d \n\r",NextState);
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunSendingCommSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunSendingCommSM(EntryEventKind);
		}
		return(ReturnEvent);
}
/****************************************************************************
 Function
     StartTemplateSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 2/18/99, 10:38AM
****************************************************************************/
void StartSendingCommSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunSendingCommSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryTemplateSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
SendingState_t QuerySendingCommSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringSendingAllBytes( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				//printf("Entered During Sending All Bytes\r\n");
        HWREG(SSI0_BASE+SSI_O_DR) = Byte2Send; //Send the first byte
				HWREG(SSI0_BASE+SSI_O_DR) = 0;
				HWREG(SSI0_BASE+SSI_O_DR) = 0;
				HWREG(SSI0_BASE+SSI_O_DR) = 0;
				HWREG(SSI0_BASE+SSI_O_DR) = 0; //Send the fifth byte

        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
      
        // repeat for any concurrent lower level machines
				if (Event.EventType == ES_EOT) { //If we are done sending all 5 bytes
					uint8_t Reading;
					Reading = HWREG(SSI0_BASE+SSI_O_DR) & 0x000000FF;
					//printf("First byte is 0x%x\r\n", Reading);
					StoreByte(Reading,1);
					Reading = HWREG(SSI0_BASE+SSI_O_DR) & 0x000000FF;
					//printf("Second byte is 0x%x\r\n", Reading);
					StoreByte(Reading,2);
					Reading = HWREG(SSI0_BASE+SSI_O_DR) & 0x000000FF;
					//printf("Third byte is 0x%x\r\n", Reading);
					StoreByte(Reading,3);
					Reading = HWREG(SSI0_BASE+SSI_O_DR) & 0x000000FF;
					//printf("Fourth byte is 0x%x\r\n", Reading);
					StoreByte(Reading,4);
					Reading = HWREG(SSI0_BASE+SSI_O_DR) & 0x000000FF;
					//printf("Fifth byte is 0x%x\r\n", Reading);
					StoreByte(Reading,5);
					//Should post here that new transaction is ready to be read
				}
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

void EOT_Response (void){
	//Post event to get into the state
	ES_Event ThisEvent;
	ThisEvent.EventType = ES_EOT;
	PostCommSM(ThisEvent);
}

/***************************************************************************
Public Helper Funcitons
 ***************************************************************************/
void StoreByte (uint8_t Reading,uint8_t ByteNumber){
	uint8_t WhichCommand = Byte2Send;
	if( (WhichCommand & CommandMask) == StatusRequest ){ // This is a Status request
		StatusArray[ByteNumber-1] = Reading;
	} else if( (WhichCommand & CommandMask) == ChangeCityRequest ){ // This is a Change request
		ChangeCityArray[ByteNumber-1] = Reading;
	} else if( (WhichCommand & CommandMask) == Query ){ // This is a Query
		QueryArray[ByteNumber-1] = Reading;
	}
}

uint8_t RR_Read(void){
	uint8_t RR;
	RR = QueryArray[2]; //return the third byte
	//printf("RR: %x \n\r",RR);
	return RR;
}
uint8_t RS_Read(void){
	uint8_t RS;
	RS = QueryArray[3]; //return the fourth byte
	//printf("RS: %x \n\r",RS); 
	return RS;
}
uint8_t SS1_Read(void){
	uint8_t SS1;
	SS1 = StatusArray[2];
	return SS1;
}
uint8_t SS2_Read(void){
	uint8_t SS2;
	SS2 = StatusArray[3];
	return SS2;
}
uint8_t SS3_Read(void){
	uint8_t SS3;
	SS3 = StatusArray[4];
	return SS3;
}