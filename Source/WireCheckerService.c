//////////////////////////////////////////////
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Driving.h"
#include "CaptureCityService.h"
#include "CommService.h"
#include "Master.h"
#include "CommandCollectorSM.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include "ADMulti.h"

#include "Driving.h"
#include "GameService.h"

#include "WireCheckerService.h"
/////////////////////////////////////////////
#define UpperThreshold 2750
#define LowerThreshold 2550
#define LOST_WIRE      1
#define FOUND_WIRE     0
#define WIRE_LOST_TH   1500

/////////////////////////////////////////////
static float CurrentAmplitudeLeft;
static float CurrentAmplitudeRight;
static float LastSeenRightWire;
static float LastSeenLeftWire;
static float Sweeping4Wire = 0;
//static uint8_t CurrentWireState = 0;
//static uint8_t LastWireState = 0;

static uint8_t MyPriority;

/////////////////////////////////////////////

static void CheckWire( void);
static void ReadWireSensors( float returnArray[2]);
	
/////////////////////////////////////////////

bool InitWireCheckerService ( uint8_t Priority )
{
  MyPriority = Priority;
  // put us into the Initial PseudoState
	ES_Timer_InitTimer(WireCheckTimer,1);
  return true;
}

bool PostWireCheckerService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

ES_Event RunWireCheckerService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors 
	if ((ThisEvent.EventType==ES_TIMEOUT) && (ThisEvent.EventParam==WireCheckTimer)){
		CheckWire(); //Check wire status and post to Navigation Service
		ES_Timer_InitTimer(WireCheckTimer, 2);
	} else if (ThisEvent.EventType==ES_Sweep4Wire) {
		Sweeping4Wire = 1;
		LastSeenRightWire = 0; //reset the state for right wire found
		LastSeenLeftWire = 0; //reset the state for left wire found
	}		
  return ReturnEvent;
}

//Helper functions

static void CheckWire( void){
	float results[2];
	static uint32_t wireLostCounter;
	static uint8_t LastWireState = FOUND_WIRE; //Default to found wire. This will allow to switch to lost wire in case we start off the wire.
	//Read wire sensor amplitude as current level
	
	if (GetCampaignState() == true) {
		ReadWireSensors(results);
		CurrentAmplitudeLeft = results[0];
		CurrentAmplitudeRight = results[1];
		
		
		

		if ((CurrentAmplitudeLeft < LowerThreshold) && (CurrentAmplitudeRight < LowerThreshold)) {
			wireLostCounter ++;
		}	
	//	else if ((CurrentAmplitudeLeft > UpperThreshold) || (CurrentAmplitudeRight > UpperThreshold)){
	//		CurrentWireState = 0;
	//	}
		if ((LastWireState == FOUND_WIRE) && (wireLostCounter > WIRE_LOST_TH)){
			//printf("Posted wire lost \r\n");
			ES_Event ThisEvent;
			ThisEvent.EventType = ES_WireLost;
			PostMasterSM(ThisEvent);
			LastWireState = LOST_WIRE;
		} else if ((LastWireState == LOST_WIRE) && ((CurrentAmplitudeLeft > UpperThreshold) || (CurrentAmplitudeRight > UpperThreshold))) {
			//printf("Posted wire found \r\n");
			wireLostCounter = 0;
			ES_Event ThisEvent;
			ThisEvent.EventType = ES_WireFound;
			PostMasterSM(ThisEvent);
			LastWireState = FOUND_WIRE;
		} else if ((LastWireState == FOUND_WIRE) && ((CurrentAmplitudeLeft > UpperThreshold) || (CurrentAmplitudeRight > UpperThreshold))) {
			wireLostCounter = 0;
	//		ES_Event ThisEvent;
	//		ThisEvent.EventType = ES_WireFound;
	//		PostMasterSM(ThisEvent);
		}
		if (Sweeping4Wire == 1) {
			if ((LastSeenRightWire == 0) && (CurrentAmplitudeRight > UpperThreshold)) {
				LastSeenRightWire = 1;
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_FoundRightWire;
				PostMasterSM(ThisEvent);
			}
			if ((LastSeenLeftWire == 0) && (CurrentAmplitudeLeft > UpperThreshold)) {
				LastSeenLeftWire = 1;
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_FoundLeftWire;
				PostMasterSM(ThisEvent);
			}
			if ((LastSeenLeftWire == 1) && (LastSeenRightWire == 1)) {
				Sweeping4Wire = 0;
			}
		} 
	}
	
	return;
}

#define alpha 0.6 //For low pass filtering
static void ReadWireSensors( float returnArray[2]) {
	static uint32_t temp[2];
	static float LeftReading = 0, RightReading = 0;
	ADC_MultiRead(temp);
	LeftReading = alpha*((float)temp[0]) + (1-alpha)*LeftReading; // Left circuit is scaled also
	RightReading = alpha*((float)((123*temp[1])-((431*4095)/33))/100) + (1-alpha)*RightReading; // Right reading is scaled due to circuit
	returnArray[0] = LeftReading;//Left Sensor
	returnArray[1] = RightReading; //Right Sensor
}

void GetWireSensorVals( float returnArray[2]) {
	returnArray[0] = CurrentAmplitudeLeft; // return left sensor reading
	returnArray[1] = CurrentAmplitudeRight; // return right sensor reading
}

bool bothWiresFound (void) {
	return ((LastSeenLeftWire == 1) && (LastSeenRightWire == 1));
}