/****************************************************************************
 Module
  CommService.c
 
----------------------------- Include Files -----------------------------*/

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"

#include "CommService.h"
#include "SendingCommSM.h"
#include "CommandCollectorSM.h"
#include "Master.h"
#include "CaptureCityService.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
static ES_Event DuringSendingComm( ES_Event Event);


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, though if the top level state machine
// is just a single state container for orthogonal regions, you could get
// away without it
static CommState_t CurrentState;
// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;
static ES_Event ThisEvent;
static uint8_t LastCommandSent;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitCommSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     boolean, False if error in initialization, True otherwise

 Description
     Saves away the priority,  and starts
     the top level state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:06
****************************************************************************/
bool InitCommSM ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;  // save our priority

  ThisEvent.EventType = ES_ENTRY;
  // Start the Master State machine

  StartCommSM( ThisEvent );

  return true;
}

/****************************************************************************
 Function
     PostCommSM

 Parameters
     ES_Event ThisEvent , the event to post to the queue

 Returns
     boolean False if the post operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostCommSM( ES_Event ThisEvent )
{
	//printf("PostCommSM Event: %d \r\n",ThisEvent.EventType);
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunCommSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   the run function for the top level state machine 
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 02/06/12, 22:09
****************************************************************************/
ES_Event RunCommSM( ES_Event CurrentEvent )
{ 
   bool MakeTransition = false;/* are we making a state transition? */
   CommState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = { ES_NO_EVENT, 0 }; // assume no error

    switch ( CurrentState )
   {
       case WaitingComm :       // If current state is state one
         // no during function
				// printf("WaitingCom \n\r");
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_SPIWrite : //If event is event one
									//printf("Got SPIWrite event\r\n"); 
									//RunCommandCollectorSM(CurrentEvent); // put this in to pass ES_SPIWrite down to commandcollector
                  // Execute action function for state one : event one
									// Post to self ES_SendCMD w/ Param1
									ThisEvent.EventType = ES_SendCMD;
									ThisEvent.EventParam = CurrentEvent.EventParam;
									PostCommSM(ThisEvent);
									//Save the last command you sent as a module level variable so you dont post each query
									LastCommandSent = CurrentEvent.EventParam;
                  NextState = SendingComm;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
                  break;
                // repeat cases as required for relevant events
            }
         }
         break;
			case SendingComm :
				CurrentEvent = DuringSendingComm(CurrentEvent); 
				switch (CurrentEvent.EventType)
				{
					case ES_TIMEOUT : 
						if (CurrentEvent.EventParam == PacTimer){ 
								NextState = WaitingComm;
								MakeTransition = true;
								ReturnEvent.EventType = ES_NO_EVENT;
								// Generate an event to indicate transfer is done
								//if the last command sent was not a query then post
								ThisEvent.EventType = ES_SPIWriteDone; //Send indicator that SPI write is done
								PostMasterSM(ThisEvent); // not sure whether or not we need this
								PostCaptureCityService(ThisEvent);
						}
					break;
				}
			break;			
      // repeat state pattern as required for other states
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunCommSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       // Execute entry function for new state
       // this defaults to ES_ENTRY
       RunCommSM(EntryEventKind);
     }
   // in the absence of an error the top level state machine should
   // always return ES_NO_EVENT, which we initialized at the top of func
   return(ReturnEvent);
}
/****************************************************************************
 Function
     StartCommSM

 Parameters
     ES_Event CurrentEvent

 Returns
     nothing

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:15
****************************************************************************/
void StartCommSM ( ES_Event CurrentEvent )
{
  // if there is more than 1 state to the top level machine you will need 
  // to initialize the state variable
  CurrentState = WaitingComm;
  // now we need to let the Run function init the lower level state machines
  // use LocalEvent to keep the compiler from complaining about unused var
  RunCommSM(CurrentEvent);
  return;
}


/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringSendingComm( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        //printf("In DuringSendingComm in high level SM\r\n");
        // after that start any lower level machines that run in this state
        StartSendingCommSM(Event);
				//StartCommandCollectorSM(Event);
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunSendingCommSM(Event);
				//RunCommandCollectorSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        RunSendingCommSM(Event);
				//RunCommandCollectorSM(Event);
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}
