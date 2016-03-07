/****************************************************************************
 Module
   Driving.c

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
#include "ADMulti.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_pwm.h"
#include "inc/hw_nvic.h"
#include "inc/hw_gpio.h"
#include "inc/hw_timer.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"	// Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "Driving.h"
#include "DrivingHelper.h"
#include "InitHWModules.h"
#include "Master.h"
#include "CaptureCityService.h"
#include "WireCheckerService.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines
#define ENTRY_STATE        Waiting2Drive

/* Defines for driving characteristics */
#define AVG_WIREFLW_SPEED  110
#define LOST_SPEED         60
#define ITERMVEL_CAP       30
#define SWEEP_RATE         30//RPM
#define RIGHT_WIRE_TH      2800
#define LEFT_WIRE_TH       2800

/* Other defines for driving */
#define DRIVING_FWD        1
#define DRIVING_BWD        -1
#define DISP_TOL           4 //Tolerance for displacement control in cm (maybe)

/* Unit Conversions */
#define TicksPerUs         40 // System ticks at 40 ticks per us 
#define TICKS2RPM          9600000 //Conversion from encoder ticks to rpm. Calculated by Michael for the new motor.

#define ALL_BITS           (0xff<<2)

#define Red 0
#define Blue 1


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringWaiting2Drive( ES_Event Event);
static ES_Event DuringDisplaceFwdBwd( ES_Event Event);
static ES_Event DuringWireFollow( ES_Event Event);
static ES_Event DuringTurning( ES_Event Event);
static ES_Event DuringSweeping( ES_Event Event);

static uint32_t abs_val(int val);

static void FollowWire( void);
static void WirePID( float, float*);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static DrivingState_t CurrentState;

static uint32_t CurrentPeriodLeft;  // Current velocity reading on left wheel
static uint32_t CurrentPeriodRight; // Current velocity reading on right wheel
static float CurrentLeftDirection;   //This will be used to tell direction and scale the readings from the encoder to be + or -
static float CurrentRightDirection;  //This will be used to tell direction and scale the readings from the encoder to be + or -
static float TargetRPMLeft=0.0;     //Target velocity on left wheel
static float TargetRPMRight=0.0;    //Target velocity on right wheel

static int CurrentLeftPosTickCount = 0;   //For counting position ticks on left wheel
static int CurrentRightPosTickCount = 0;  //For counting position ticks on right wheel
static float TargetPositionLeft = 0.0;    //Target displacement on left wheel
static float TargetPositionRight = 0.0;   //Target displacement on right wheel


static float CurrentAmplitudeLeft;    //Current reading on left wire sensor
static float CurrentAmplitudeRight;   //Current reading on right wire sensor

static uint8_t VelControlEnable;         	//Enable velocity control
static uint8_t PosControlEnable;	      	//Enable position control
static uint8_t BatteryLowIndicatorEnable;	//Enable battery low indicator when we are on wire follow

// Defines for motor velocity PID
static float motorVelKp = 0.5;
static float motorVelKd = 0.0;
static float motorVelKi = 0.0005;
static float CurrentSpeed; //for wire following

// Defines for motor position PID
static float motorPosKp = 1.5;
static float motorPosKd = 0.01;
static float motorPosKi = 0.005;

// Defines for wire PID
static float wireKp = 0.3;
static float wireKd = 0.01;
static float wireKi = 0.001;
static float RequestedDutyLeft, RequestedDutyRight;

//Defines for if you hit wall
static uint8_t LeftCounter = 0;
static uint8_t RightCounter = 0;


static uint8_t TeamColor;

/*---------------------------- Module Constants ---------------------------*/

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunDrivingSM

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
ES_Event RunDrivingSM( ES_Event CurrentEvent )
{	 
	 //printf("In DrivingSM \n\r");
   bool MakeTransition = false;/* are we making a state transition? */
   DrivingState_t NextState = CurrentState; //Assume that there will be no transition first
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

	 if (CurrentEvent.EventType == ES_TurnRight ) {
		 printf("Got turn right event\r\n");
	 }
   switch ( CurrentState )
   {
				case Waiting2Drive :       // If current state is state one
//					 printf("In Waiting2Drive \n\r");
					 // Execute During function for state one. ES_ENTRY & ES_EXIT are
					 // processed here allow the lower level state machines to re-map
					 // or consume the event
					 CurrentEvent = DuringWaiting2Drive(CurrentEvent);
					 //process any events
					 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
					 {
							switch (CurrentEvent.EventType)
							{
								 case ES_FinishedDriving:
									 ReturnEvent.EventType = ES_NO_EVENT;
								 break;
								 case ES_WireFollow : //If event is command to follow wire
										if (CurrentEvent.EventParam == 1) {
											CurrentSpeed = LOST_SPEED;
										} else if (CurrentEvent.EventParam == 2) {
											CurrentSpeed = AVG_WIREFLW_SPEED;
										}
										// Execute action function for state one : event one
										NextState = WireFollow;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										//EntryEventKind.EventType = ES_ENTRY_HISTORY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										ReturnEvent.EventType = ES_NO_EVENT;
									break;
									// repeat cases as required for relevant events
//								 case ES_FindWire : Driving should not get FindWire event
//									 printf("Transitioning to wire follow\r\n");
//										NextState = DrivingForward;
//										MakeTransition = true;
//										//Post the same event to self to get to drive forward
//										ES_Event ThisEvent;
//										ThisEvent.EventType = ES_FindWire;
//										PostMasterSM(ThisEvent);
//										ReturnEvent.EventType = ES_NO_EVENT;
//								 break;
								 case ES_DriveForward :
									  NextState = DisplaceFwdBwd;
										MakeTransition = true;
										// Take in the displacement parameters
										TargetPositionLeft = CurrentEvent.EventParam;
										TargetPositionRight = CurrentEvent.EventParam;
										ReturnEvent.EventType = ES_NO_EVENT;
								 break;
								 case ES_DriveBackward :
									  NextState = DisplaceFwdBwd;
										MakeTransition = true;
										TargetPositionLeft = -CurrentEvent.EventParam;
										TargetPositionRight = -CurrentEvent.EventParam;
										ReturnEvent.EventType = ES_NO_EVENT;
								    
								 break;
								 case ES_TurnRight :
									 printf("Got turn right event\r\n");
									  NextState = Turning;
										MakeTransition = true;
										//Take in the turning parameters
										TargetPositionLeft = (CurrentEvent.EventParam*400)/1800; //Convert degrees into cm to travel TODO: will need to tune this
										TargetPositionRight = -(CurrentEvent.EventParam*400)/1800;
										ReturnEvent.EventType = ES_NO_EVENT;
								 break;
								 case ES_TurnLeft :
									  NextState = Turning;
										MakeTransition = true;
										//Take in the turning parameters
										TargetPositionLeft = -(CurrentEvent.EventParam*400)/1800; //Convert degrees into cm to travel TODO: will need to tune this
										TargetPositionRight = (CurrentEvent.EventParam*400)/1800; //379
										ReturnEvent.EventType = ES_NO_EVENT;
								 break;
							}
					 }
         break;
					 
				 // repeat state pattern as required for other states
				 case WireFollow :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
         CurrentEvent = DuringWireFollow(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_StopMotors :
                  // Execute action function for state one : event one
                  NextState = Waiting2Drive;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  //EntryEventKind.EventType = ES_ENTRY_HISTORY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
                  break;
                // repeat cases as required for relevant events
							 case ES_SweepWire:
								 printf("Got ES_SweepWire\r\n");
								 NextState = Sweeping;
								 MakeTransition = true;
								 ReturnEvent.EventType = ES_NO_EVENT;
							 break;
							 case ES_TIMEOUT :
									if (CurrentEvent.EventParam == DrivingTimer){
										// Execute action function for state one : event one
										NextState = WireFollow;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = false; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										//EntryEventKind.EventType = ES_ENTRY_HISTORY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										ReturnEvent.EventType = ES_NO_EVENT;
								 }
                  break;					 
            }
         }
         break;
					 
				 case DisplaceFwdBwd:
					 CurrentEvent = DuringDisplaceFwdBwd(CurrentEvent);
					 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
						{
							switch (CurrentEvent.EventType)
							{
								case ES_FinishedDriving : 
										 NextState = Waiting2Drive;//Decide what the next state will be
											// for internal transitions, skip changing MakeTransition
											MakeTransition = true; //mark that we are taking a transition
											//We do not want to consume this event since navigation needs it
								break;
								case ES_TIMEOUT :
									    if (CurrentEvent.EventParam == DrivingTimer) {
												// Make sure we continue in our current state
												NextState = DisplaceFwdBwd;//Decide what the next state will be
												MakeTransition = false; //mark that we are taking a transition
												ReturnEvent.EventType = ES_NO_EVENT;
											}
								break; 
								case ES_DriveForward :
												// Make sure we continue in our current state
									     	NextState = DisplaceFwdBwd;//Decide what the next state will be
												MakeTransition = false; //mark that we are taking a transition
												ReturnEvent.EventType = ES_NO_EVENT;
								break;
								case ES_DriveBackward :
												// Make sure we continue in our current state
									     	NextState = DisplaceFwdBwd;//Decide what the next state will be
												MakeTransition = false; //mark that we are taking a transition
												ReturnEvent.EventType = ES_NO_EVENT;
								break;
							}
						 }
				 break;
						 
				case Turning:
					 CurrentEvent = DuringTurning(CurrentEvent);
					 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
					 {
					   switch (CurrentEvent.EventType)
						 {
							 case ES_FinishedDriving :
										 NextState = Waiting2Drive;//Decide what the next state will be
											// for internal transitions, skip changing MakeTransition
											MakeTransition = true; //mark that we are taking a transition
											//We do not want to consume this event since navigation needs it
								break;
//							  
								case ES_TIMEOUT :
									    if (CurrentEvent.EventParam == DrivingTimer) {
												// Make sure we continue in our current state
												NextState = Turning;//Decide what the next state will be
												MakeTransition = false; //mark that we are taking a transition
												ReturnEvent.EventType = ES_NO_EVENT;
											}
								break;
								case ES_TurnLeft :
												// Make sure we continue in our current state
									     	NextState = Turning;//Decide what the next state will be
												MakeTransition = false; //mark that we are taking a transition
												ReturnEvent.EventType = ES_NO_EVENT;
								break;
								case ES_TurnRight :
												// Make sure we continue in our current state
									     	NextState = Turning;//Decide what the next state will be
												MakeTransition = false; //mark that we are taking a transition
												ReturnEvent.EventType = ES_NO_EVENT;
								break;
						 }
					 }
					 break;
					 case Sweeping:
						CurrentEvent = DuringSweeping(CurrentEvent);
						if (CurrentEvent.EventType == ES_SweepingDone) {
							NextState = Waiting2Drive;
							MakeTransition = true;
							//Do not consume event and pass it to navigation
						} else if (CurrentEvent.EventType == ES_StopMotors) {
							NextState = Waiting2Drive;
							MakeTransition = true;
							CurrentEvent.EventType = ES_NO_EVENT; //Consumed
						}
					break;
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       // Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunDrivingSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunDrivingSM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartDrivingSM

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
void StartDrivingSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
	 TeamColor = OurTeamColor();
 
   // call the entry function (if any) for the ENTRY_STATE
   RunDrivingSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryDrivingSM

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
DrivingState_t QueryDrivingSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringWaiting2Drive( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				//Stop the motors
				PosControlEnable = 0;
				VelControlEnable = 0;
				SetDutyRight(0);
				SetDutyLeft(0);
				TargetRPMLeft = 0;
				TargetRPMRight = 0;
			
				printf("In Waiting2Drive \n\r");
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
				VelControlEnable = 0;
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
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}



static ES_Event DuringWireFollow( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
		
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
			printf("In WireFollow\r\n");
			ES_Timer_InitTimer(DrivingTimer, 10);
			PosControlEnable = 0; //Make sure position control is disabled first
			VelControlEnable = 1;
			//BatteryLowIndicatorEnable = 1;
    }
    else if ( Event.EventType == ES_EXIT )
    {
			VelControlEnable = 0;
			//BatteryLowIndicatorEnable = 0;
			// Turn back on the team LED
			//TurnOnTeamColor();
			
    }else
    // do the 'during' function for this state
    {
        // do any activity that is repeated as long as we are in this state
			FollowWire();

			if ((Event.EventType == ES_TIMEOUT) && (Event.EventParam == OneShotLeftTimer)) {
					CurrentPeriodLeft = TICKS2RPM;
				  
			
				//Start the one shot timer to know if you hit a wall. Note: This is stopped and then restarted in the encoder interrupt  
				ES_Timer_InitTimer(OneShotLeftTimer,20);	
				LeftCounter++;
				//printf("LeftCounter: %d\r\n",LeftCounter);
				
				
		  }
		  if ((Event.EventType == ES_TIMEOUT) && (Event.EventParam == OneShotRightTimer)) {
					CurrentPeriodRight = TICKS2RPM;
				//Start the one shot timer to know if you hit a wall. Note: This is  
				ES_Timer_InitTimer(OneShotRightTimer,20);
				RightCounter++;
				//printf("RightCounter: %d\r\n",RightCounter);
					
		  }
			//printf("RPM on right req duty %f\r\n", RequestedDutyRight);
			//printf("RPM on right encoder %d and left encoder %d\r\n", TICKS2RPM/CurrentPeriodRight, TICKS2RPM/CurrentPeriodLeft);
		  //printf("Target RPM on right %f Target RPM on left %f, error %f\r\n", TargetRPMRight, TargetRPMLeft, (float)(CurrentAmplitudeRight-CurrentAmplitudeLeft));
			//printf("Right Amplitude %d and Left Amplitude %d\r\n", CurrentAmplitudeRight, CurrentAmplitudeLeft);
			ES_Timer_InitTimer(DrivingTimer, 2);
    
			//Check if we are stuck at wall
			if ((LeftCounter > 10)&& (RightCounter > 10)){
					ES_Event ThisEvent;
					ThisEvent.EventType = ES_Stuck;
					PostMasterSM(ThisEvent);
					LeftCounter = 0;
					RightCounter = 0;
			}
		
		
		}
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringDisplaceFwdBwd( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
			printf("In DuringDisplaceFwdBwd\r\n");
			CurrentLeftPosTickCount = 0; //Reset the count on encoders to do the new displacement
			CurrentRightPosTickCount = 0;
			
			VelControlEnable = 0; //Make sure velocity control is disabled first
			PosControlEnable = 1;
			ES_Timer_InitTimer(DrivingTimer, 10);
    }
    else if ( Event.EventType == ES_EXIT )
    {
			PosControlEnable = 0;      
    } else
    // do the 'during' function for this state
    {
			float CurrentPosLeft, CurrentPosRight;
      // do any activity that is repeated as long as we are in this state

			CurrentPosLeft = (((float)CurrentLeftPosTickCount)*237)/2500;
			CurrentPosRight = (((float)CurrentRightPosTickCount)*237)/2500;

			//printf("Pos err Left %d, Pos err Right %d\r\n", CurrentLeftPosTickCount, CurrentRightPosTickCount);
			//printf("target Left %f, target Right %f, Pos Left %f, Pos Right %f\r\n", TargetPositionLeft, TargetPositionRight, CurrentPosLeft, CurrentPosRight);
			if ((abs_val(CurrentPosLeft - TargetPositionLeft) < DISP_TOL) 
						&& (abs_val(CurrentPosRight - TargetPositionRight) < DISP_TOL)) { //Then we are at target
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_FinishedDriving;
				PostMasterSM(ThisEvent);
			} else {
				ES_Timer_InitTimer(DrivingTimer, 10); // Else continue checking that we are done
			}

    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringTurning( ES_Event Event)
{
	ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
			printf("In DuringTurning\r\n");
			CurrentLeftPosTickCount = 0; //Reset the count on encoders to do the new displacement
			CurrentRightPosTickCount = 0;
			
			VelControlEnable = 0; //Make sure velocity control is disabled first
			PosControlEnable = 1;
			ES_Timer_InitTimer(DrivingTimer, 10);
    }
    else if ( Event.EventType == ES_EXIT )
    {
			PosControlEnable = 0;      
    } else
    // do the 'during' function for this state
    {

			float CurrentPosLeft, CurrentPosRight;
      // do any activity that is repeated as long as we are in this state

			CurrentPosLeft = (((float)CurrentLeftPosTickCount)*237)/2500;
			CurrentPosRight = (((float)CurrentRightPosTickCount)*237)/2500;

			//printf("Pos err Left %d, Pos err Right %d\r\n", CurrentLeftPosTickCount, CurrentRightPosTickCount);
			//printf("target Left %f, target Right %f, Pos Left %f, Pos Right %f\r\n", TargetPositionLeft, TargetPositionRight, CurrentPosLeft, CurrentPosRight);
			if ((abs_val(CurrentPosLeft - TargetPositionLeft) < DISP_TOL) 
						&& (abs_val(CurrentPosRight - TargetPositionRight) < DISP_TOL)) { //Then we are at target
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_FinishedDriving;
				PostMasterSM(ThisEvent);
			} else {
				ES_Timer_InitTimer(DrivingTimer, 10); // Else continue checking that we are done
			}

    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringSweeping( ES_Event Event) {
		ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
		static uint8_t TurningDir; //0 for Right 1 for Left
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
		ES_Event ThisEvent;
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
			printf("In DuringSweeping\r\n");
			PosControlEnable = 0; //Make sure position control is disabled first
			VelControlEnable = 1;
			TargetRPMRight = SWEEP_RATE;
			TargetRPMLeft = -SWEEP_RATE;
			TurningDir = 1;
			//BatteryLowIndicatorEnable = 1;
    }
    else if ( Event.EventType == ES_EXIT )
    {
			VelControlEnable = 0;	
    }else
    // do the 'during' function for this state
    {
			if ((Event.EventType == ES_FoundRightWire) && (TurningDir == 1)) {
				if (bothWiresFound()) {
					ThisEvent.EventType = ES_SweepingDone;
					PostMasterSM(ThisEvent);
//					printf("posted sweeping done turning left\r\n");
				} else {
					TurningDir = 0;
					TargetRPMRight = -SWEEP_RATE; //change direction turn right
					TargetRPMLeft = SWEEP_RATE;
//					printf("Changed sweep dir to right\r\n");
				}
			}
			if ((Event.EventType == ES_FoundLeftWire) && (TurningDir == 1)) {
				TurningDir = 1;
				TargetRPMRight = SWEEP_RATE; //continue turning left
				TargetRPMLeft = -SWEEP_RATE;
//				printf("Saw first wire and continued Left\r\n");
			}
			if ((Event.EventType == ES_FoundLeftWire) && (TurningDir == 0)) {
				if (bothWiresFound()) {
					ThisEvent.EventType = ES_SweepingDone;
					PostMasterSM(ThisEvent);
//					printf("posted sweeping done turning right\r\n");
				} else {
					TurningDir = 1;
					TargetRPMRight = SWEEP_RATE; //change direction turn left
					TargetRPMLeft = -SWEEP_RATE;
//					printf("Changed sweep dir to left\r\n");
				}
			}
			if ((Event.EventType == ES_FoundRightWire) && (TurningDir == 0)) {
				TurningDir = 1;
				TargetRPMRight = -SWEEP_RATE; // change direction turning right
				TargetRPMLeft = SWEEP_RATE;
//				printf("Saw first wire and continued Right\r\n");
			}		
		}
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
	
}


/****************************************************************************
 Function
     FollowWire

 Parameters
     None

 Returns
     None

 Description
     runs the FollowWire algorithm. Toggles the direction of the bot to 
follow maintain the wire centered.
 Notes

 Author
     Team 13, 2/11/05, 10:38AM
****************************************************************************/
static void FollowWire( void){
	static float RPM_array[2];
	static float results[2];
	//Read light sensor amplitude as current level
	GetWireSensorVals(results);
	CurrentAmplitudeLeft = results[0];
	CurrentAmplitudeRight = results[1];
	WirePID((CurrentAmplitudeRight-CurrentAmplitudeLeft), RPM_array);
	TargetRPMLeft =  RPM_array[0]; // RPM function takes in a percentage of max RPM
	TargetRPMRight = RPM_array[1];
}

/****************************************************************************
 Function
     WirePID

 Parameters
     int16_t error between the peaks of the two wire sensor readings. RIGHT_SENSOR - LEFT_SENSOR
		 uint8_t* array where we right the calculated duty cycles

 Returns
     none

 Description
     Implements the PID algorithm for following wire
 Notes

 Author
     Team 13, 2/11/05, 10:38AM
****************************************************************************/
void WirePID( float err , float RPM_array[2]){
	static float pTerm, dTerm, iTerm;
	static float pidTermLeft, pidTermRight;
	static float lastError = 0; /* for Derivative Control */
	
	
	pTerm = err*wireKp;
	dTerm = (err - lastError)*wireKd;
	iTerm += wireKi * err;
	if (iTerm > 80){
		iTerm = 80;
	}
	if (iTerm < -80){
		iTerm = -80;
	}  /* anti-windup */
	
	iTerm = 0; //TODO comment out later for i term
	pidTermLeft = CurrentSpeed+(pTerm+dTerm+iTerm);
	pidTermRight = CurrentSpeed-(pTerm+dTerm+iTerm);
	
	if (pidTermLeft > 250){ //Limit our RPM to 110 to protect the motors
		RPM_array[0] = 250;
	} else if (pidTermLeft < -250) {
		RPM_array[0] = -250;
	} else {
		RPM_array[0] = (int) pidTermLeft;
	}
	if (pidTermRight > 250){
		RPM_array[1] = 250;
	} else if (pidTermRight < -250) {
		RPM_array[1] = -250;
	} else {
		RPM_array[1] = (int) pidTermRight;
	}
	
	lastError = err; //save the last error
}


/****************************************************************************
 Function
     InputCaptureResponseLeft

 Parameters
     none

 Returns
     none

 Description
     Captures the period on the Left encoder to be used for velocity control
 Notes

 Author
     Team 13, 2/11/05, 10:38AM
****************************************************************************/
void InputCaptureResponseLeft( void ){
	static uint32_t LastCaptureLeft;
	uint32_t ThisCapture1;
// start by clearing the source of the interrupt, the input capture event
	HWREG(WTIMER3_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
	ES_Timer_StopTimer(OneShotLeftTimer);
	
	if (PosControlEnable == 1) {
		if (CurrentLeftDirection == DRIVING_FWD) {
			CurrentLeftPosTickCount ++;
		} else if (CurrentLeftDirection == DRIVING_BWD) {
			CurrentLeftPosTickCount --;
		}
	}

// now grab the captured value and calculate the period
	ThisCapture1 = HWREG(WTIMER3_BASE+TIMER_O_TAR);
	CurrentPeriodLeft = ThisCapture1 - LastCaptureLeft;
// update LastCapture to prepare for the next edge
	LastCaptureLeft = ThisCapture1;
	ES_Timer_InitTimer(OneShotLeftTimer, 20);
	
	//Clear Left Counter to see if you hit wall
	LeftCounter = 0;
	

}

/****************************************************************************
 Function
     InputCaptureResponseRight

 Parameters
     none

 Returns
     none

 Description
     Captures the period on the Right encoder to be used for velocity control
 Notes

 Author
     Team 13, 2/11/05, 10:38AM
****************************************************************************/
void InputCaptureResponseRight( void ){
	static uint32_t LastCaptureRight;
	uint32_t ThisCapture2;
// start by clearing the source of the interrupt, the input capture event
	HWREG(WTIMER1_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
	ES_Timer_StopTimer(OneShotRightTimer);
	
	if (PosControlEnable == 1) {
		if (CurrentRightDirection == DRIVING_FWD) {
			CurrentRightPosTickCount ++;
		} else if (CurrentRightDirection == DRIVING_BWD) {
			CurrentRightPosTickCount --;
		}
	}
	
// now grab the captured value and calculate the period
	ThisCapture2 = HWREG(WTIMER1_BASE+TIMER_O_TAR);
	CurrentPeriodRight = ThisCapture2 - LastCaptureRight;
// update LastCapture to prepare for the next edge
	LastCaptureRight = ThisCapture2;
	ES_Timer_InitTimer(OneShotRightTimer, 20);
	
	
	//Clear Right Counter to see if you hit wall
	RightCounter = 0;
}

#if 0 //deprecated, now using ES_Timers instead
//This timer is used to tell whether there has been a period of time without edges on the inputCaptureResponseLeft
void OneShotTimeOutLeft( void ){
// start by clearing the source of the interrupt, the Timer1 TimerA timeout
	HWREG(WTIMER1_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
	//If we got this interrupt it means that our wire sensor read no pulse in input, so indicate that with a value of 0
	CurrentPeriodLeft = TICKS2RPM;
	oneShotSetLeft = false;
}

//This timer is used to tell whether there has been a period of time without edges on the inputCaptureResponseRight
void OneShotTimeOutRight( void ){
// start by clearing the source of the interrupt, the Timer1 TimerA timeout
	HWREG(WTIMER2_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
	//If we got this interrupt it means that our wire sensor read no pulse in input, so indicate that with a value of 0
	CurrentPeriodRight = TICKS2RPM;
	oneShotSetRight = false;
}
#endif

/****************************************************************************
 Function
     MotorVelocityPID

 Parameters
     none

 Returns
     none

 Description
     Interrupt service routine to perform velocity control for motors.
 Notes

 Author
     Team 13, 2/11/05, 10:38AM
****************************************************************************/
void MotorVelocityPID( void ){
	static float iTermVelLeft=0.0, iTermVelRight=0.0; /* integrator control effort */
	static float RPMErrorLeft, RPMErrorRight; /* make static for speed */
	static float LastVelErrorLeft = 0, LastVelErrorRight = 0; /* for Derivative Control */
	static float RPMLeft, RPMRight;
	
	// start by clearing the source of the interrupt
	HWREG(WTIMER0_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
	if (VelControlEnable) {
		RPMLeft = CurrentLeftDirection*((float) (TICKS2RPM/CurrentPeriodLeft)); // actual RPM measured by encoder
		RPMRight = CurrentRightDirection*((float) (TICKS2RPM/CurrentPeriodRight)); // actual RPM measured by encoder
		RPMErrorLeft = TargetRPMLeft - RPMLeft;
		RPMErrorRight = TargetRPMRight - RPMRight;

		iTermVelLeft += motorVelKi * RPMErrorLeft;
		iTermVelRight += motorVelKi * RPMErrorRight;

		if (iTermVelLeft > ITERMVEL_CAP){
			iTermVelLeft = ITERMVEL_CAP;
		} else if (iTermVelLeft < -ITERMVEL_CAP){
			iTermVelLeft = -ITERMVEL_CAP;
		} /* anti-windup */
		if (iTermVelRight > ITERMVEL_CAP){
			iTermVelRight = ITERMVEL_CAP;
		} else if (iTermVelRight < -ITERMVEL_CAP){
			iTermVelRight = -ITERMVEL_CAP;
		} /* anti-windup */
		
		RequestedDutyLeft =
		(motorVelKp*(RPMErrorLeft)+iTermVelLeft+(motorVelKd*(RPMErrorLeft-LastVelErrorLeft)));
		RequestedDutyRight =
		(motorVelKp*(RPMErrorRight)+iTermVelRight+(motorVelKd*(RPMErrorRight-LastVelErrorRight)));
		
		
		if (RequestedDutyLeft > 100){
			RequestedDutyLeft = 100;
		}
		if (RequestedDutyRight > 100){
			RequestedDutyRight = 100;
		}
		if (RequestedDutyLeft < -100){
			RequestedDutyLeft = -100;
		}
		if (RequestedDutyRight < -100){
			RequestedDutyRight = -100;
		}
		LastVelErrorLeft = RPMErrorLeft; // update last error
		LastVelErrorRight = RPMErrorRight; // update last error
		
		// We need to figure out our current direction since there is no quadruture encoder
		if (RequestedDutyLeft >= 0) { //EncReadingSign will be used to scale the readings from the encoder to be + or - 
			CurrentLeftDirection = DRIVING_FWD;
		} else {
			CurrentLeftDirection = DRIVING_BWD;
		}
		if (RequestedDutyRight >= 0) { //EncReadingSign will be used to scale the readings from the encoder to be + or - 
			CurrentRightDirection = DRIVING_FWD;
		} else {
			CurrentRightDirection = DRIVING_BWD;
		}
		SetDutyLeft((int) RequestedDutyLeft); //SetDuty for both driving directions on left wheel
		SetDutyRight((int) RequestedDutyRight); //SetDuty for both driving directions on right wheel
	}
}

/****************************************************************************
 Function
     MotorPositionPID

 Parameters
     none

 Returns
     none

 Description
     Interrupt service routine to perform position control for motors.
 Notes

 Author
     Team 13, 2/11/05, 10:38AM
****************************************************************************/
void MotorPositionPID( void )
{
	static float iTermPosLeft=0.0, iTermPosRight=0.0; /* integrator control effort */
	static float PosErrorLeft, PosErrorRight;         /* make static for position */
	static float LastPosErrorLeft = 0, LastPosErrorRight = 0; /* for Derivative Control */
	static float CurrentPosLeft, CurrentPosRight;
	
	
	HWREG(TIMER0_BASE+TIMER_O_ICR) = TIMER_ICR_TATOCINT;
	// start by clearing the source of the interrupt
	if (PosControlEnable) {
		CurrentPosLeft = (((float)CurrentLeftPosTickCount)*237)/2500; //Convert the ticks to cm travelled
		CurrentPosRight = (((float)CurrentRightPosTickCount)*237)/2500; //Convert the ticks to cm travelled
		PosErrorLeft = TargetPositionLeft - CurrentPosLeft;
		PosErrorRight = TargetPositionRight - CurrentPosRight;

		iTermPosLeft += motorPosKi * PosErrorLeft;
		iTermPosRight += motorPosKi * PosErrorRight;
		if (iTermPosLeft > 20){
			iTermPosLeft = 20;
		} else if (iTermPosLeft < -20) {
			iTermPosLeft = -20;
		} /* anti-windup */
		if (iTermPosRight > 20){
			iTermPosRight = 20;
		} else if (iTermPosRight < -20){
			iTermPosRight = -20;
		} /* anti-windup */
		
		RequestedDutyLeft =
		(motorPosKp*(PosErrorLeft)+iTermPosLeft+(motorPosKd*(PosErrorLeft-LastPosErrorLeft)));
		RequestedDutyRight =
		(motorPosKp*(PosErrorRight)+iTermPosRight+(motorPosKd*(PosErrorRight-LastPosErrorRight)));
		
		if (RequestedDutyLeft > 100){
			RequestedDutyLeft = 100;
		} else if (RequestedDutyLeft < -100){
			RequestedDutyLeft = -100;
		}
		if (RequestedDutyRight > 100){
			RequestedDutyRight = 100;
		} else if (RequestedDutyRight < -100){
			RequestedDutyRight = -100;
		}
		LastPosErrorLeft = PosErrorLeft; // update last error
		LastPosErrorRight = PosErrorRight; // update last error

		// We need to figure out our current direction since there is no quadruture encoder
		if (RequestedDutyLeft >= 0) { //EncReadingSign will be used to scale the readings from the encoder to be + or - 
			CurrentLeftDirection = DRIVING_FWD;
		} else {
			CurrentLeftDirection = DRIVING_BWD;
		}
		if (RequestedDutyRight >= 0) { //EncReadingSign will be used to scale the readings from the encoder to be + or - 
			CurrentRightDirection = DRIVING_FWD;
		} else {
			CurrentRightDirection = DRIVING_BWD;
		}
		SetDutyLeft((int) RequestedDutyLeft); //SetDuty for both driving directions on left wheel
		SetDutyRight((int) RequestedDutyRight); //SetDuty for both driving directions on right wheel
	}
}
	

//static float alpha = 0.4;
//static float LeftSensor_i = 0;
//static float RightSensor_i = 0;


//	void ReadWireSensors( float returnArray[2]) {
//	static uint32_t temp[2];
//	ADC_MultiRead(temp);
//	float LeftSensorScaled = (float) temp[0];
//	float RightSensorScaled = (float) (((123*temp[1])-((431*4095)/33))/100); //Right sensor need scaling
////	returnArray[0] = temp[0]; //Left sensor 
////	returnArray[1] = ((20*temp[1])-((255*4095)/33))/6; //Right sensor need scaling
//	
//	
//	//Filtered Values y(i)= alpha*x(i)+(1-alpha)*y(i-1)
//	LeftSensor_i = alpha*LeftSensorScaled + (1-alpha)*LeftSensor_i;
//	RightSensor_i = alpha*RightSensorScaled + (1-alpha)*RightSensor_i;
//	returnArray[0] = LeftSensor_i;
//	returnArray[1] = RightSensor_i;
//	
//	
//}

static uint32_t abs_val(int val) {
	return (val >= 0)? val: -val;
}
