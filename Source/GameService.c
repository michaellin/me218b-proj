/****************************************************************************
 Module
   GameService.c

 Revision
   2.0.1

 Description
   This is a template file for implementing state machines.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/07/13 21:00 jec      corrections to return variable (should have been
                         ReturnEvent, not CurrentEvent) and several EV_xxx
                         event names that were left over from the old version
 02/08/12 09:56 jec      revisions for the Events and Services Framework Gen2
 02/13/10 14:29 jec      revised Start and run to add new kind of entry function
                         to make implementing history entry cleaner
 02/13/10 12:29 jec      added NewEvent local variable to During function and
                         comments about using either it or Event as the return
 02/11/10 15:54 jec      more revised comments, removing last comment in during
                         function that belongs in the run function
 02/09/10 17:21 jec      updated comments about internal transitions on During funtion
 02/18/09 10:14 jec      removed redundant call to RunLowerlevelSM in EV_Entry
                         processing in During function
 02/20/07 21:37 jec      converted to use enumerated type for events & states
 02/13/05 19:38 jec      added support for self-transitions, reworked
                         to eliminate repeated transition code
 02/11/05 16:54 jec      converted to implment hierarchy explicitly
 02/25/03 10:32 jec      converted to take a passed event parameter
 02/18/99 10:19 jec      built template from MasterMachine.c
 02/14/99 10:34 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "GameService.h"
#include "Master.h"
#include "CaptureCityService.h"
#include "CommService.h"
#include "SendingCommSM.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include "driverlib/gpio.h"
#include "LEDService.h"
#include "InitHWModules.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE Waiting4Campaign


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringWaiting4Campaign( ES_Event Event);
static ES_Event DuringCampaigning_StoppingAtCity( ES_Event Event); //This one is for state Campaigning_StopAtCity
static ES_Event DuringCampaigning_NotStoppping( ES_Event Event);
static ES_Event DuringShootingInCampaign( ES_Event Event);
static void RequestGameStatus(void);
static void GetGameStatus(void);
static ES_Event DuringCampaigning_CapturingCity( ES_Event);


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static GameServiceState_t CurrentState;
static uint8_t SS1;
static uint8_t SS2;
static uint8_t SS3;
static uint8_t GS_MASK = 0x01;
static bool InCampaign = false;
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunGameServiceSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 2/11/05, 10:45AM
****************************************************************************/
ES_Event RunGameServiceSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   GameServiceState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
	 ES_Event ThisEvent;
	 //printf("In Game service \n\r");
	

   switch ( CurrentState )
   {
       case Waiting4Campaign :       // If current state is state one
						 // Execute During function for state one. ES_ENTRY & ES_EXIT are
						 // processed here allow the lower level state machines to re-map
						 // or consume the event
						 CurrentEvent = DuringWaiting4Campaign(CurrentEvent);
						 //process any events
						 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
						 {
								switch (CurrentEvent.EventType)
								{
									 case ES_StartCampaign : //If event is event one
											
											NextState = Campaigning_StopAtCity;//Decide what the next state will be
											// for internal transitions, skip changing MakeTransition
											MakeTransition = true; //mark that we are taking a transition
											// if transitioning to a state with history change kind of entry
											//EntryEventKind.EventType = ES_ENTRY_HISTORY;
											// optionally, consume or re-map this event for the upper
											// level state machine
											
											//start status timer and do your first SPI_request status
											//ES_Timer_InitTimer(GameStatusTimer, 200);
											//RequestGameStatus();
									 
											ReturnEvent.EventType = ES_NO_EVENT;
											break;
										// repeat cases as required for relevant events
								}
						 }
         break;
      // repeat state pattern as required for other states
				 case Campaigning_StopAtCity :       // If current state is state one
							 // Execute During function for state one. ES_ENTRY & ES_EXIT are
							 // processed here allow the lower level state machines to re-map
							 // or consume the event
							 CurrentEvent = DuringCampaigning_StoppingAtCity(CurrentEvent);
							 //process any events
							 
							 
							 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
							 {
									switch (CurrentEvent.EventType)
									{
										 case ES_AtCity : //If event is event one
												// Execute action function for state one : event one
													//printf("ES_AtCity in campaigning stop at city \r\n");
													
													//First stop the motors
													ThisEvent.EventType = ES_StopMotors;
													PostMasterSM(ThisEvent); 
										 
													//Switch your next state to Campaigning Capturing City
													NextState = Campaigning_CapturingCity;
													MakeTransition = true;
									
													//First Post to yourself in the Campaigning Capturing city state and then post to capture city service from there 
													//This is a safety so you dont querry the game status while trying to capture the city.
													//PostAtCity to this service and then in next state post it toCaptureCity service
													ThisEvent.EventType = ES_StartCaptureCity;
													ThisEvent.EventParam = CurrentEvent.EventParam;
													PostMasterSM(ThisEvent);
													//PostCaptureCityService(ThisEvent); //do this in next state
													
													
											break;
										 
										 ////////////////////////////////////////////////////
										 //Moved this stuff down to Campaigning_CapturingCity
										 ////////////////////////////////////////////////////
//											case ES_IgnoreCity : //Posted by capture city service and possibly others at some point
//												// Execute action function for state one : event one									
//												//	ThisEvent.EventType = ES_WireFollow; //TRY: take out this line cause navigation may take care of it already especially if we get an ignore city from another source that doesnt want to wirefollow
//												//	PostMasterSM(ThisEvent);
//													NextState = Campaigning_DontStopAtCity;
//													MakeTransition = true;
//													//PostCaptureCityService(CurrentEvent); //TRY: If things dont work try posting this in Campaigning_DontStopAtCity if we get ES_AtCity also - but we shouldnt have to post there i think
//								
//											break;
//												
//											case ES_BallShot : //If event is event one
//												
//												//Just stay in the next state to ignore ES_AtCity events and dont post them to driving 
//												NextState = ShootingInCampaign;
//												MakeTransition = true; 
//											
//											break;
												
												
											// repeat cases as required for relevant events
										 case ES_GameOver : //If event is event one
												// Execute action function for state one : event one
												NextState = Waiting4Campaign;//Decide what the next state will be
												// for internal transitions, skip changing MakeTransition
												MakeTransition = true; //mark that we are taking a transition
												//Consume Event
												ReturnEvent.EventType = ES_NO_EVENT;
												break;
										 case ES_WireFound:
											 //Update our strategy now that we are on the wire
											 break;
										 case ES_WireLost:
											 //Update our strategy now that we lost the wire
											 break;
											// repeat cases as required for relevant events
									}
							 }
         break;
				 
				case Campaigning_CapturingCity :
							 CurrentEvent = DuringCampaigning_CapturingCity(CurrentEvent);
							 //process any events
									switch (CurrentEvent.EventType)
									{
												
											case ES_StartCaptureCity :
													//Tell Capture city service to start 
													ThisEvent.EventType = ES_StartCaptureCity;
													ThisEvent.EventParam = CurrentEvent.EventParam;
													PostCaptureCityService(ThisEvent);		
											break;
										
										
										 case ES_GameOver : //If event is event one
												// Execute action function for state one : event one
												NextState = Waiting4Campaign;//Decide what the next state will be
												// for internal transitions, skip changing MakeTransition
												MakeTransition = true; //mark that we are taking a transition
												//Consume Event
												ReturnEvent.EventType = ES_NO_EVENT;
											break;
										 
										 case ES_IgnoreCity : //Posted by capture city service and possibly others at some point
												// Execute action function for state one : event one									
												//	ThisEvent.EventType = ES_WireFollow; //TRY: take out this line cause navigation may take care of it already especially if we get an ignore city from another source that doesnt want to wirefollow
												//	PostMasterSM(ThisEvent);
													NextState = Campaigning_DontStopAtCity;
													MakeTransition = true;
													//PostCaptureCityService(CurrentEvent); //TRY: If things dont work try posting this in Campaigning_DontStopAtCity if we get ES_AtCity also - but we shouldnt have to post there i think
								
											break;
										 case ES_TIMEOUT:
											 if (CurrentEvent.EventParam==MakeSureNotStuckTimer){
												 NextState = Campaigning_DontStopAtCity;
												 MakeTransition = true;
												 //Try sending a query to restart your capture service.
												 ES_Event ThisEvent;
												 ThisEvent.EventType = ES_SPIWrite;
												 ThisEvent.EventParam = 0x70; //Query
												 PostCaptureCityService(ThisEvent);
												 
											 }
											break;
											case ES_BallShot : //If event is event one
												
												//Just stay in the next state to ignore ES_AtCity events and dont post them to driving 
												NextState = ShootingInCampaign;
												MakeTransition = true; 
												printf("Go to shooting in GameService \r\n");
											
											break;
											
											case ES_AtCity : //If event is event one
												// Execute action function for state one : event one
													//printf("ES_AtCity in campaigning stop at city \r\n");
													
//													//First stop the motors
//													ThisEvent.EventType = ES_StopMotors;
//													PostMasterSM(ThisEvent); 
										 
													//First Post to yourself in the Campaigning Capturing city state and then post to capture city service from there 
													//This is a safety so you dont querry the game status while trying to capture the city.
													//PostAtCity to this service and then in next state post it toCaptureCity service
													ThisEvent.EventType = ES_StartCaptureCity;
													ThisEvent.EventParam = CurrentEvent.EventParam;
													PostCaptureCityService(ThisEvent);
													//PostCaptureCityService(ThisEvent); //do this in next state													
											break;
										 
										 
										 
									 } //End Event Switch
					break; //break Campaigning_CapturingCity
											 
				 
				 case ShootingInCampaign :      
						//CurrentEvent = DuringCampaigning_StoppingAtCity(CurrentEvent); //probably should take this out
						//process any events
						if ( CurrentEvent.EventType == ES_WireFollow){ //If an event is active
							//Go back to Campaigning state
							//NextState = Campaigning_StopAtCity; TODO: if below state doesnt work put it back
//							
								//Made my own timer event instead of going to dont stop at city
//							NextState = Campaigning_DontStopAtCity;
//							MakeTransition = true; 
							
							//start timer for skipping city after shooting
							ES_Timer_InitTimer(CampaignTimer,1000);
						} 
						
						else if ((CurrentEvent.EventType == ES_TIMEOUT)&&(CurrentEvent.EventParam==CampaignTimer)){
							printf("Got timeout on IgnoreCity \r\n");
							NextState = Campaigning_DontStopAtCity;
							MakeTransition = true; 
						}
						
						else if ( CurrentEvent.EventType ==ES_GameOver){ //If event is event one
												// Execute action function for state one : event one
												NextState = Waiting4Campaign;//Decide what the next state will be
												// for internal transitions, skip changing MakeTransition
												MakeTransition = true; //mark that we are taking a transition
												//Consume Event
												ReturnEvent.EventType = ES_NO_EVENT;
						 }
         break;
						
					
				 case Campaigning_DontStopAtCity :      
						CurrentEvent = DuringCampaigning_NotStoppping( CurrentEvent); //The entry for this starts the Campaign Timer
						//printf("DontStopAtCity Event: %d, Param %d\n\r",CurrentEvent.EventType,CurrentEvent.EventParam); 
						//process any events
						//If we get not at city event or we get a timeout then go back to stopping at cities state
						//if ((CurrentEvent.EventType == ES_NotAtCity)||((ThisEvent.EventType == ES_TIMEOUT)&&(ThisEvent.EventParam==CampaignTimer))){
						if ((CurrentEvent.EventType == ES_TIMEOUT)&&(CurrentEvent.EventParam==CampaignTimer)){
							printf("Got timeout on IgnoreCity \r\n");
							NextState = Campaigning_StopAtCity;
							MakeTransition = true; 
						}
						else if ( CurrentEvent.EventType == ES_GameOver){ //If event is event one
												// Execute action function for state one : event one
												NextState = Waiting4Campaign;//Decide what the next state will be
												// for internal transitions, skip changing MakeTransition
												MakeTransition = true; //mark that we are taking a transition
												//Consume Event
												ReturnEvent.EventType = ES_NO_EVENT;
						 }
         break;	
								
					
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunGameServiceSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunGameServiceSM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartGameServiceSM

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
void StartGameServiceSM ( ES_Event CurrentEvent )
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
   RunGameServiceSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryGameServiceSM

 Parameters
     None

 Returns
     GameServiceState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
GameServiceState_t QueryGameServiceSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringWaiting4Campaign( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {		
				InCampaign = false;
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_Celebrate;
				PostLEDService(ThisEvent);
        // implement any entry actions required for this state machine
				printf("Entered Waiting for campaign \r\n"); 
				RequestGameStatus();
				ES_Timer_InitTimer(GameStatusTimer, 200);
				ThisEvent.EventType = ES_StopMotors;
				PostMasterSM(ThisEvent);
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
			   
			
			//Start with wirefollow - may change to findwire later
 			 InitGameOneShot();
				 ES_Event ThisEvent;
				 ThisEvent.EventType = ES_WireFollow;
				 ThisEvent.EventParam = 2;
				 PostMasterSM(ThisEvent);
				 uint8_t myTeamColor = OurTeamColor();
					ThisEvent.EventType = ES_StopCelebrate;
					PostLEDService(ThisEvent); 
			
			//printf("here\r\n");
				if (myTeamColor == 1) {
					 SetBlueLED();
				 } else if (myTeamColor == 0) {
					 SetRedLED();
				 }
				 
				 InCampaign = true;
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
			ES_Event ThisEvent;
			ThisEvent.EventType = ES_StopMotors;
			//PostMasterSM(ThisEvent);                        //I took this out
			if ((Event.EventType == ES_TIMEOUT) && (Event.EventParam == GameStatusTimer)){
					GetGameStatus();
				
				
				if ((SS3 & GS_MASK) == 0x01){
					
					printf("here\r\n");
					ThisEvent.EventType = ES_StartCampaign;
					PostMasterSM(ThisEvent);
				}
				else{
					RequestGameStatus();
					ES_Timer_InitTimer(GameStatusTimer, 200);
				}
			}
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringCampaigning_StoppingAtCity( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
			printf("During Campaign Stopping at cities\r\n");
			HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + (0xff<<2))) |= GPIO_PIN_3;
        // implement any entry actions required for this state machine
        
//			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//Starts Driving in Wirefollow mode -  may want to move somewhere else later
			//Post Wirefollow event to Driving
//				printf("Entered During Campainging\r\n");
//				ES_Event ThisEvent;
//				ThisEvent.EventType = ES_WireFollow; //TODO put this back after debugging turning
//				PostMasterSM(ThisEvent);
//      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			  //			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//Starts Driving in WireFinding mode (DriveForwad)-  may want to move somewhere else later
			//Post Wirefollow event to Driving
//				ES_Event ThisEvent;
//			  ThisEvent.EventType = ES_FindWire;
//				PostMasterSM(ThisEvent);
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			
			
			// after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
			HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + (0xff<<2))) &= ~GPIO_PIN_3;
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
       //if there is a timeout for the game status timer then check the status, restart the timer, and if the game ended then post ES_GameOver
//				if ((Event.EventType == ES_TIMEOUT)&&(Event.EventParam == GameStatusTimer)){
//				//GetGameStatus();
//				//RequestGameStatus();
////						if ((SS3 & GS_MASK) == 0x00){
////						ES_Event ThisEvent;
////						ThisEvent.EventType = ES_GameOver;
////						PostMasterSM(ThisEvent);
////						}else{
////						//Restart the GameStatus timer
////							ES_Timer_InitTimer(GameStatusTimer,200);
////						}
//				} //End seeing if game is over
				
						
		}
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringCampaigning_CapturingCity( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
			printf("During Campaign Capturing City\r\n");
        // implement any entry actions required for this state machine
			ES_Timer_InitTimer(MakeSureNotStuckTimer, 4000);
//      ES_Timer_StopTimer(GameStatusTimer);
			HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + (0xff<<2))) |= (GPIO_PIN_3 | GPIO_PIN_2);//yellow

    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				ES_Timer_StopTimer(MakeSureNotStuckTimer);
//				ES_Timer_InitTimer(GameStatusTimer, 200);
				HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + (0xff<<2))) &= ~(GPIO_PIN_3 | GPIO_PIN_2); //yellow
      
    }else
    // do the 'during' function for this state
    {
//       //Restart the timer but dont post anything to the pac
//			if ((Event.EventType == ES_TIMEOUT)&&(Event.EventParam==GameStatusTimer)){
//				//Restart the timer but do not querry so you dont mess with pac while we're capturing city
//				ES_Timer_InitTimer(GameStatusTimer, 200);
//			}
				
						
		}
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringCampaigning_NotStoppping( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
				//Start a timer to get us back to Campaigning and waiting to stop state
			  printf("During Campaign Not stopping at cities\r\n");
				ES_Timer_InitTimer(CampaignTimer, 1500);
			//Note: This makes it less general. In the future if we want to do something other than wire follow we should put ourselves back to our last state and not default to wire follow
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_WireFollow;
			  ThisEvent.EventParam = 2;
				PostMasterSM(ThisEvent);
				HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + (0xff<<2))) |= (GPIO_PIN_3 | GPIO_PIN_1);
			
				//Post to capture city service so it goes back to waiting state now
				ThisEvent.EventType = ES_OkToWaitCapture;
				PostCaptureCityService(ThisEvent);
			// after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + (0xff<<2))) &= ~(GPIO_PIN_3 | GPIO_PIN_1);
      
    }else
    // do the 'during' function for this state
    {
        //if there is a timeout for the game status timer then check the status, restart the timer, and if the game ended then post ES_GameOver
				if ((Event.EventType == ES_TIMEOUT)&&(Event.EventParam == GameStatusTimer)){
				//GetGameStatus();
				//RequestGameStatus();
//						if ((SS3 & GS_MASK) == 0x00){
//						ES_Event ThisEvent;
//						ThisEvent.EventType = ES_GameOver;
//						PostMasterSM(ThisEvent);
//						}else{
//						//Restart the GameStatus timer
//							ES_Timer_InitTimer(GameStatusTimer,200);
//						}
     		} //End seeing if game is over
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


static ES_Event DuringShootingInCampaign( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        //dont do anything on entry
    }
    else if ( Event.EventType == ES_EXIT )
    {
        //dont do anything on exit
      
    }else
    // do the 'during' function for this state
    {
        //if there is a timeout for the game status timer then check the status, restart the timer, and if the game ended then post ES_GameOver
				if ((Event.EventType == ES_TIMEOUT)&&(Event.EventParam == GameStatusTimer)){
				//GetGameStatus();
				//RequestGameStatus();
//						if ((SS3 & GS_MASK) == 0x00){
//						ES_Event ThisEvent;
//						ThisEvent.EventType = ES_GameOver;
//						PostMasterSM(ThisEvent);
//						}else{
//						//Restart the GameStatus timer
//							ES_Timer_InitTimer(GameStatusTimer,200);
//						}
				} //End seeing if game is over
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static void RequestGameStatus(void){
	ES_Event ThisEvent;
	ThisEvent.EventType = ES_SPIWrite;
	ThisEvent.EventParam = 0xC0; // Status byte
	PostCommSM(ThisEvent);
}

static void GetGameStatus(void){
	SS1 = SS1_Read();
	SS2 = SS2_Read();
	SS3 = SS3_Read();
	//printf("SS3 %x \r\n",SS3);
}

void GameEndedResponse( void ){
	HWREG(WTIMER4_BASE+TIMER_O_ICR) = TIMER_ICR_TATOCINT;
	printf("Game ended\r\n");
	ES_Event ThisEvent;
	ThisEvent.EventType = ES_GameOver;
	PostMasterSM(ThisEvent);
}

bool GetCampaignState( void){
	return InCampaign;
}