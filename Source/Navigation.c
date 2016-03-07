/****************************************************************************
 Module
  Navigation.c

 Revision
   2.0.1

 
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Navigation.h"
#include "Driving.h"
#include "GameService.h"
#include "CaptureCityService.h"
#include "Master.h"
#include "HSMTemplate.h"
#include "ShootingService.h"
#include "WireCheckerService.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_timer.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE DrivingState
#define Clockwise 1
#define CounterClockwise 2
#define UnknownDirection 0


#define Red 0
#define Blue 1
//Adjust these for the shootig offset movement (also can change forwards or backwards in SendShootingOffsetCoordinates function

#define SeattleClockwiseAngle 70 // Left
#define SeattleClockwiseLinearMove 38 //Backward
#define SeattleCounterClockwiseAngle 63 // Right
#define SeattleCounterClockwiseLinearMove 38 //Backward
#define MiamiClockwiseAngle 110 //Left
#define MiamiClockwiseLinearMove 40 //Backward
#define MiamiCounterClockwiseAngle 57 //Right
#define MiamiCounterClockwiseLinearMove 40 //Backward

#define SWEEP_WIRE_TIME 8000 //Give it 5 seconds to sweep for wire

#define ALL_BITS   (0xff<<2)
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringDrivingState( ES_Event Event);
static ES_Event DuringWireLostDrivingState( ES_Event Event);
static ES_Event DuringWireLostSweeping( ES_Event Event);
//static ES_Event DuringFindingWire( ES_Event Event);
static uint8_t UpdateCityDirection( void);
void SendShootingAngle( uint8_t ThisCity, uint8_t Direction);
void SendShootingPosition( uint8_t ThisCity, uint8_t Direction);
void ReturnShootingPosition( uint8_t ThisCity, uint8_t Direction);
void ReturnShootingAngle( uint8_t ThisCity, uint8_t Direction);
static ES_Event DuringStuckState_Turn( ES_Event Event);
static ES_Event DuringStuckState_Backup( ES_Event Event);



/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static NavigationState_t CurrentState;
static uint8_t CurrentCity;
static uint8_t LastCity;
static uint8_t DrivingDirection = Clockwise;
static uint8_t CurrentCityIndex;
static int LastCityIndex = -1;
static uint8_t CityDirection; //0 for unkown, 1 clockwise, 2 counterclockwise
//Cities in order of occurance in a clockwise loop starting at CA 
static uint8_t CityArray[9] = {0x01, 0x02, 0x03,0x04,0x06,0x09,0x08,0x07,0x05 };
static uint8_t TeamColor; //TeamColor = OurTeamColor(void); //Red = 0, blue = 1
static uint8_t OnWire = 1; //Start as indicating that we are on wire

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunNavigationSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   This state machine should implement all navigation methods (e.g. wire following, off-road)
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 2/11/05, 10:45AM
****************************************************************************/
ES_Event RunNavigationSM( ES_Event CurrentEvent )
{
   //printf("In Navigation \n\r");
   bool MakeTransition = false; /* are we making a state transition? */
   NavigationState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
   ES_Event ThisEvent;
   if (CurrentEvent.EventType == ES_WireFound) {
     OnWire = 1;
   } else if (CurrentEvent.EventType == ES_WireLost) {
     OnWire = 0;
   }
   switch ( CurrentState )
   {
       case DrivingState :       // If navigation is in driving state (Wire Follow or Finding)
         //Call during function first
         CurrentEvent = DuringDrivingState(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType) {
              case ES_GotCityName: 
                CurrentCity = CurrentEvent.EventParam;
                printf("GOT City Name: %x \r\n",CurrentCity);
                
                //Get our team color
                TeamColor = OurTeamColor(); //Red = 0, Blue = 1
                
                //Find Index of the current city
                for (int i = 0; i<9; i++){
                    if (CurrentCity == CityArray[i]){
                        CurrentCityIndex = i;
                        break;
                    }
                }
                //Store City to figure out direction
                CityDirection = UpdateCityDirection();
                if (CityDirection == Clockwise){
                  printf("CLOCKWISE\n\r");
                } else if (CityDirection == CounterClockwise){
                  printf("CounterClockwiseE\n\r");
                } else if (CityDirection == UnknownDirection) {
                  printf("UNKOWN DIRECTION\n\r");
                }
                LastCityIndex = CurrentCityIndex;
          
                
                //If we are at a shooting city
                if (((CurrentCity == 0x02) &&(TeamColor == Blue)) || ((CurrentCity == 0x07)&&(TeamColor == Red))){
                  if (CityDirection != UnknownDirection) {
                    //Make sure we are in Waiting2Drive
                    ES_Event ThisEvent;
                    ThisEvent.EventType = ES_StopMotors;
                    PostMasterSM(ThisEvent);
                    //Tell driving to angle the bot for shooting
                    printf("SendShootingAngle \r\n");
                    SendShootingAngle( CurrentCity, CityDirection); 
                    //If the direction is unknown then drive to next city, and do a 180 turn 
                    //Change the curerent state to Offset2ShootState
                    NextState = SendingShootingAngleState;
                    MakeTransition = true;
                  }
                  
                }
                
                //If currrent city not Seattle or Miami
                else {
                  
                  //Stay in Driving state 
                  
                  //Enable WireFollow
                  ES_Event ThisEvent;  
                  ThisEvent.EventType = ES_WireFollow;
                  ThisEvent.EventParam = 2; // indicate to go normal speed
                  PostMasterSM(ThisEvent); //This might be a problem. Do we always want to wire follow if city is not seattle of miami?
                }
              break;
              case ES_WireLost: //If we lost the wire while finding wire
                NextState = WireLostDriving; //Go on a slow turn move to find the wire
                ThisEvent.EventType = ES_StopMotors;
                PostMasterSM(ThisEvent);
                MakeTransition = true;
                EntryEventKind.EventType = ES_ENTRY_HISTORY;
                ReturnEvent.EventType = ES_NO_EVENT; //Consume this event
              break;
              
              case ES_Stuck:
                  //Go to StuckState
                  printf("Navigation Got ES_Stuck \r\n");
                  ThisEvent.EventType = ES_StopMotors;
                  PostMasterSM(ThisEvent);
                  NextState = StuckState_Backup;
                  MakeTransition = true;
                  EntryEventKind.EventType = ES_ENTRY_HISTORY; //Use history in Driving
              break;
            }
          }
         break;
         
      case SendingShootingAngleState :
          CurrentEvent = DuringDrivingState(CurrentEvent);
          //printf("SendShootingAngleState\r\n");
          if (CurrentEvent.EventType == ES_FinishedDriving){
              //Tell driving to move into position for shooting
              printf("SendShootingPosition\r\n");
              SendShootingPosition( CurrentCity, CityDirection);
              NextState = SendingShootingPositionState;
              MakeTransition = true;
              EntryEventKind.EventType = ES_ENTRY_HISTORY; //Use history in Driving
              ReturnEvent.EventType = ES_NO_EVENT; // We consume this event since Master does not need it
          }
      break;
      
      case SendingShootingPositionState :    
        CurrentEvent = DuringDrivingState(CurrentEvent);  
        //printf("SendShootingPositionState\r\n");
        if (CurrentEvent.EventType == ES_FinishedDriving){
            //You are now in position: Tell the shooting state machine to pull the trigger!
            ES_Event ThisEvent;
            printf("SendStartShooting\r\n");
            ThisEvent.EventType = ES_StartShooting;
            PostShootingService(ThisEvent);
            NextState = WaitingForReloadState;
            MakeTransition = true;
            EntryEventKind.EventType = ES_ENTRY_HISTORY; //Use history in Driving
            ReturnEvent.EventType = ES_NO_EVENT; // We consume this event since Master does not need it
            
        }
      break;
      
      
      case WaitingForReloadState :
          CurrentEvent = DuringDrivingState(CurrentEvent);
          if (CurrentEvent.EventType == ES_BallShot){
            //Set the next state to Returning2Position
            NextState = Returning2PositionState;
            MakeTransition = true;
            //Possibly post to GameService that you finished Shooting  
            //Return by go opposite of your old position
            printf("SendReturnPosition\r\n");
            if (CurrentCity == 0x02){
            ReturnShootingPosition( CurrentCity, CityDirection);
            }
            else if (CurrentCity == 0x07){
            ReturnShootingAngle( CurrentCity, CityDirection);
            }
            ReturnEvent.EventType = ES_NO_EVENT; // We consume this event since Master does not need it
          }
      break;
           
           
      case Returning2PositionState :    
          CurrentEvent = DuringDrivingState(CurrentEvent);
          if (CurrentEvent.EventType == ES_FinishedDriving){
            //Set the next state to Returning2Angle
            NextState = Returning2AngleState;
            MakeTransition = true;
            EntryEventKind.EventType = ES_ENTRY_HISTORY; //Use history in Driving
            //Return by go opposite of your old angle
            printf("SendReturnAngle\r\n");
            //ReturnShootingAngle for CA and position for miami
            if (CurrentCity == 0x02){
              ReturnShootingAngle( CurrentCity, CityDirection);
            }
            else if (CurrentCity == 0x07){
              ReturnShootingPosition( CurrentCity, CityDirection);
            }
            ReturnEvent.EventType = ES_NO_EVENT; // We consume this event since Master does not need it
          } 
      break;
      
      case Returning2AngleState :     
          CurrentEvent = DuringDrivingState(CurrentEvent);
          if (CurrentEvent.EventType == ES_FinishedDriving){
            //Set the next state to DrivingState
            NextState = DrivingState;
            MakeTransition = true;
            EntryEventKind.EventType = ES_ENTRY_HISTORY; //Use history in Driving
            //Set CurrentCity to 16 so that this doesnt read it again
            CurrentCity = 16;
            //Tell driving to follow wire
            printf("Back to Wirefollow");
            ES_Event ThisEvent;
            ThisEvent.EventType = ES_WireFollow;
            ThisEvent.EventParam = 2;
            PostMasterSM(ThisEvent);
            ReturnEvent.EventType = ES_NO_EVENT; // We consume this event since Master does not need it
          } 
      break;
      case WireLostDriving:
        CurrentEvent = DuringWireLostDrivingState(CurrentEvent); 
        if (CurrentEvent.EventType == ES_WireFound) {
          //ES_Timer_InitTimer(WireFindingTimer, HIGH_GAIN_TIME); //Give it some time to find the wire
//          ThisEvent.EventType = ES_StopMotors;
//          PostMasterSM(ThisEvent);
          ES_Timer_InitTimer(WireFindingTimer, SWEEP_WIRE_TIME); //Give it some time to sweep
//          printf("saw wire, attempting to sweep\r\n");
          NextState = WireLostSweeping;
          MakeTransition = true;
          EntryEventKind.EventType = ES_ENTRY_HISTORY; //Use history in Driving
          ReturnEvent.EventType = ES_NO_EVENT;
        } else if (CurrentEvent.EventType == ES_Stuck) {
          ThisEvent.EventType = ES_StopMotors;
          PostMasterSM(ThisEvent);
          NextState = StuckState_Backup;
          MakeTransition = true;
          EntryEventKind.EventType = ES_ENTRY_HISTORY; //Use history in Driving
          ReturnEvent.EventType = ES_NO_EVENT;
        }
      break;
      case WireLostSweeping:
        CurrentEvent = DuringWireLostSweeping(CurrentEvent); 
        if (CurrentEvent.EventType == ES_SweepingDone) {
          //ES_Timer_InitTimer(WireFindingTimer, HIGH_GAIN_TIME); //Give it some time to find the wire
          ThisEvent.EventType = ES_WireFollow; //Before it was high gain wire follow
          ThisEvent.EventParam = 2;
          PostMasterSM(ThisEvent);
          //printf("posted es high gain wire follow to driving\r\n");
          //NextState = GettingOnWire;
          NextState = DrivingState;
          MakeTransition = true;
          EntryEventKind.EventType = ES_ENTRY_HISTORY; //Use history in Driving
          ReturnEvent.EventType = ES_NO_EVENT;
        } else if ((CurrentEvent.EventType == ES_TIMEOUT) && (CurrentEvent.EventParam == WireFindingTimer)) { //If we time out then give up and go back to driving
//          printf("Giving up on sweeping\r\n");
          ThisEvent.EventType = ES_StopMotors;
          PostMasterSM(ThisEvent);
          NextState = WireLostDriving;
          MakeTransition = true;
          EntryEventKind.EventType = ES_ENTRY_HISTORY; //Use history in Driving
          ReturnEvent.EventType = ES_NO_EVENT;
        }
      break;
      case StuckState_Backup:
        //Backup
        
        CurrentEvent = DuringStuckState_Backup(CurrentEvent); //Sends drive backward event in entry and starts DrivingSM
        
        //Wait for ES Finished driving 
        if (CurrentEvent.EventType == ES_FinishedDriving){
          NextState = StuckState_Turn;
          MakeTransition = true;
          EntryEventKind.EventType = ES_ENTRY_HISTORY; //Use history in Driving
        }
        
      break;
        
      case StuckState_Turn :
        //Turn
        CurrentEvent = DuringStuckState_Turn(CurrentEvent); //Sends turn event in entry and starts drivingSM
        
        //Go back to wire follow - TODO: If wire finding works then go into that instead
        //Wait for ES Finished driving 
        if (CurrentEvent.EventType == ES_FinishedDriving){
          //Go back to drivingState
          if (OnWire) {
            NextState = DrivingState;
          } else {
            NextState = WireLostDriving; //If we are not on wire then go to this
          }
          MakeTransition = true;
          EntryEventKind.EventType = ES_ENTRY_HISTORY; //Use history in Driving
          ES_Event ThisEvent;
          //Post WireFollow to driving
//          printf("Now Go to wirefollow \r\n");
          ThisEvent.EventType = ES_WireFollow;
          if (OnWire) {
            ThisEvent.EventParam = 2;
          } else {
            ThisEvent.EventParam = 1;
          }
          PostMasterSM(ThisEvent);
        }
        break;
      
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
//         printf("Made Nav state transistion\r\n");
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
//       printf("ES_Exit function in nav\r\n");
       RunNavigationSM(CurrentEvent);
       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
//       printf("ES_Entry function in nav\r\n");
       RunNavigationSM(EntryEventKind);
     }
     return(ReturnEvent);
   
}
/****************************************************************************
 Function
     StartNavigationSM

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
void StartNavigationSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
   TeamColor = OurTeamColor(); //Determine our team color at the beginning
   // call the entry function (if any) for the ENTRY_STATE
   RunNavigationSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryNavigationSM

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
NavigationState_t QueryNavigationSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringDrivingState( ES_Event Event) //During driving state is basically during wire following
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        printf("Entered Driving State\r\n");
        // after that start any lower level machines that run in this state
        StartDrivingSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunDrivingSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        ReturnEvent = RunDrivingSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


static ES_Event DuringStuckState_Backup( ES_Event Event) //During driving state is basically during wire following
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        printf("Entered StuckState_Backup\r\n");
        //Post drive backward to drivingSM
        ES_Event ThisEvent;
        ThisEvent.EventType = ES_DriveBackward;
        ThisEvent.EventParam = 10;
        PostMasterSM(ThisEvent);
      
        // after that start any lower level machines that run in this state
        StartDrivingSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunDrivingSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        ReturnEvent = RunDrivingSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringStuckState_Turn( ES_Event Event) //During driving state is basically during wire following
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        
      
        // after that start any lower level machines that run in this state
        StartDrivingSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
      
      //Post drive backward to drivingSM
        ES_Event ThisEvent;
        ThisEvent.EventType = ES_TurnRight; 
        ThisEvent.EventParam = 180;
        PostMasterSM(ThisEvent);
      printf("Entered StuckState_Turn\r\n");
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunDrivingSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        ReturnEvent = RunDrivingSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringFindingWire( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        printf("Entered Slow turning to find wire\r\n");
        // after that start any lower level machines that run in this state
        StartDrivingSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunDrivingSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
      
        ReturnEvent = RunDrivingSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringWireLostDrivingState( ES_Event Event) {
  ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        printf("Entered Wire Lost Driving State\r\n");
        // after that start any lower level machines that run in this state
        StartDrivingSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
        ES_Event ThisEvent;
        ThisEvent.EventType = ES_WireFollow;
        ThisEvent.EventParam = 1; // indicate to go slower
        PostMasterSM(ThisEvent);
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunDrivingSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
      
        ReturnEvent = RunDrivingSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


static ES_Event DuringWireLostSweeping( ES_Event Event) {
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        printf("Entered Sweep Wire State\r\n");
        // after that start any lower level machines that run in this state
        StartDrivingSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
        ES_Event ThisEvent;
        ThisEvent.EventType = ES_SweepWire;
        PostMasterSM(ThisEvent);
        ThisEvent.EventType = ES_Sweep4Wire;
        PostWireCheckerService(ThisEvent);
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunDrivingSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
      
        ReturnEvent = RunDrivingSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

//Function to update city direction
static uint8_t UpdateCityDirection( void){
  if (LastCityIndex == -1){
    CityDirection = UnknownDirection;
  }
  else if (((((9+CurrentCityIndex)-LastCityIndex))%9) > ((((9+LastCityIndex)-CurrentCityIndex))%9)){
    CityDirection = CounterClockwise;
  }
  else if (((((9+CurrentCityIndex)-LastCityIndex))%9) < ((((9+LastCityIndex)-CurrentCityIndex))%9)){
    CityDirection = Clockwise;
  }
  return CityDirection;
}

void SendShootingAngle( uint8_t ThisCity, uint8_t Direction){
  //if the direction is unknown
  if (Direction == UnknownDirection){
    //Probably should do something else - but im just going to guess that its counterclockwise
    Direction  = CounterClockwise;
  }
  
  //If the city is Seattle
  ES_Event ThisEvent;
  if (ThisCity == 0x02){
    if (Direction == Clockwise){
      //Post the angle to turn
      ThisEvent.EventType = ES_TurnLeft; //Option: ES_TurnRight
      ThisEvent.EventParam = SeattleClockwiseAngle;
      PostMasterSM(ThisEvent);
    }
    else if (Direction == CounterClockwise){
      //Post the angle to turn
      ThisEvent.EventType = ES_TurnRight; //Option: ES_TurnRight
      ThisEvent.EventParam = SeattleCounterClockwiseAngle;
      PostMasterSM(ThisEvent);
    }
  }
  else if (ThisCity == 0x07){
    if (Direction == Clockwise){
      //Post the angle to turn
      ThisEvent.EventType = ES_TurnLeft; //Option: ES_TurnRight
      ThisEvent.EventParam = MiamiClockwiseAngle;
      PostMasterSM(ThisEvent);
    }
    else if (Direction == CounterClockwise){
      //Post the angle to turn
      ThisEvent.EventType = ES_TurnRight; //Option: ES_TurnRight
      ThisEvent.EventParam = MiamiCounterClockwiseAngle;
      PostMasterSM(ThisEvent);
    }
  }
}

void SendShootingPosition( uint8_t ThisCity, uint8_t Direction){
  //if the direction is unknown
  if (Direction == UnknownDirection){
  //Probably should do something else - but im just going to guess that its counterclockwise
  Direction  = CounterClockwise;
  }
  
  //If the city is Seattle
  ES_Event ThisEvent;
  if (ThisCity == 0x02){
    if (Direction == Clockwise){
      //Post the direction to turn
      ThisEvent.EventType = ES_DriveBackward; //Option: ES_DriveForward
      ThisEvent.EventParam = SeattleClockwiseLinearMove ;
      PostMasterSM(ThisEvent);
    }
    else if (Direction == CounterClockwise){
      //Post the direction to turn
      ThisEvent.EventType = ES_DriveBackward; //Option: ES_DriveForward
      ThisEvent.EventParam = SeattleCounterClockwiseLinearMove ;
      PostMasterSM(ThisEvent);
    }
  }
  else if (ThisCity == 0x07){
    if (Direction == Clockwise){
      //Post the direction to turn
      ThisEvent.EventType =ES_DriveBackward; //Option: ES_DriveForward
      ThisEvent.EventParam = MiamiClockwiseLinearMove ;
      PostMasterSM(ThisEvent);
    }
    else if (Direction == CounterClockwise){
      //Post the direction to turn
      ThisEvent.EventType = ES_DriveBackward; //Option: ES_DriveForward
      ThisEvent.EventParam = MiamiCounterClockwiseLinearMove ;
      PostMasterSM(ThisEvent);
    }
  }
}

void ReturnShootingPosition( uint8_t ThisCity, uint8_t Direction){
  //if the direction is unknown
  if (Direction == UnknownDirection){
    //Probably should do something else - but im just going to guess that its counterclockwise
    Direction  = CounterClockwise;
  }
  
  //If the city is Seattle
  ES_Event ThisEvent;
  if (ThisCity == 0x02){
      if (Direction == Clockwise){
          //Post the direction to turn
          ThisEvent.EventType = ES_DriveForward; //Option: ES_DriveBackward (Do Opposite of the going to position ones)
          ThisEvent.EventParam = SeattleClockwiseLinearMove  ;
          PostMasterSM(ThisEvent);
          
      }
      else if (Direction == CounterClockwise){
          //Post the direction to turn
          ThisEvent.EventType = ES_DriveForward; //Option: ES_DriveBackward (Do Opposite of the going to position ones)
          ThisEvent.EventParam = SeattleCounterClockwiseLinearMove ;
          PostMasterSM(ThisEvent);
          
      }
  }
  else if (ThisCity == 0x07){
      if (Direction == Clockwise){
          //Post the direction to turn
          ThisEvent.EventType = ES_DriveForward; //Option: ES_DriveBackward (Do Opposite of the going to position ones)
          ThisEvent.EventParam = MiamiClockwiseLinearMove - 15;
          PostMasterSM(ThisEvent);
      }
      else if (Direction == CounterClockwise){
          //Post the direction to turn
          ThisEvent.EventType = ES_DriveForward; //Option: ES_DriveBackward (Do Opposite of the going to position ones)
          ThisEvent.EventParam = MiamiCounterClockwiseLinearMove ;
          PostMasterSM(ThisEvent);
      }
  }
}

void ReturnShootingAngle( uint8_t ThisCity, uint8_t Direction){
  //if the direction is unknown
  if (Direction == UnknownDirection){
  //Probably should do something else - but im just going to guess that its counterclockwise
  Direction  = CounterClockwise;
  }
  
  //If the city is Seattle
  ES_Event ThisEvent;
  if (ThisCity == 0x02){
    if (Direction == Clockwise){
        //Post the angle to turn
        ThisEvent.EventType = ES_TurnRight; //Option: ES_TurnLeft (Make sure it is opposite from the getting into position one)
        ThisEvent.EventParam = SeattleClockwiseAngle+15;
        PostMasterSM(ThisEvent);
    }
    else if (Direction == CounterClockwise){
        //Post the angle to turn
        ThisEvent.EventType = ES_TurnLeft; //Option: ES_TurnRight (Make sure it is opposite from the getting into position one)
        ThisEvent.EventParam = SeattleCounterClockwiseAngle;
        PostMasterSM(ThisEvent);
    }
  }
  else if (ThisCity == 0x07){
    if (Direction == Clockwise){
        //Post the angle to turn
        ThisEvent.EventType = ES_TurnRight; //Option: ES_TurnRight (Make sure it is opposite from the getting into position one)
        ThisEvent.EventParam = MiamiClockwiseAngle+50;
        PostMasterSM(ThisEvent);
    }
    else if (Direction == CounterClockwise){
        //Post the angle to turn
        ThisEvent.EventType = ES_TurnLeft; //Option: ES_TurnRight (Make sure it is opposite from the getting into position one)
        ThisEvent.EventParam = MiamiCounterClockwiseAngle +60;
        PostMasterSM(ThisEvent);
    }
  }
}
  
