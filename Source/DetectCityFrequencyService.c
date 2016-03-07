/****************************************************************************
 Module
DetectCityFrequencyService.c

Description:
Flat state machine to detect city frequency
****************************************************************************/

/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "CaptureCityService.h"
#include "DetectCityFrequencyService.h"
#include "CaptureCityService.h"
#include "Master.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_timer.h"

/*---------------------------- Module Defines ---------------------------*/
#define FREQ_TOL   30 //City frequency tolerance
#define TicksPerUs 40 // 40 ticks per us
#define ALL_BITS   (0xff<<2)

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
static DetectCityFrequency_t CurrentState;
static uint32_t CityPeriod; //Current city period updated by input capture
static uint32_t ValidCityPeriod;
static uint32_t LastValidCityPeriod;
static uint8_t  CandidateCityValue = 16; //just use a value different from ReturnCityValue

/*---------------------------- Module Constants ---------------------------*/
static const uint32_t ValidCityFreqs[16] = {TicksPerUs*1333, TicksPerUs*1277, TicksPerUs*1222, TicksPerUs*1166,
											TicksPerUs*1111, TicksPerUs*1055, TicksPerUs*1000, TicksPerUs*944,
											TicksPerUs*889, TicksPerUs*833, TicksPerUs*778, TicksPerUs*722,
											TicksPerUs*667, TicksPerUs*611, TicksPerUs*556, TicksPerUs*500};

/*---------------------------- Module Function ---------------------------*/
static uint8_t CheckCityPeriod(void);

/****************************************************************************
 Function
    InitDetectCityFrequencyService

****************************************************************************/
bool InitDetectCityFrequencyService ( uint8_t Priority )
{
  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = InitCityFrequency;
	ES_Event ThisEvent;
	ThisEvent.EventType = ES_INIT;
	PostDetectCityFrequencyService(ThisEvent);
  return true;
}

/****************************************************************************
 Function
     PostDetectCityFrequencyService

 
****************************************************************************/
bool PostDetectCityFrequencyService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}


/****************************************************************************
 Function
    RunDetectCityFrequencyService

 
****************************************************************************/
ES_Event RunDetectCityFrequencyService( ES_Event ThisEvent )
{
	ES_Event ReturnEvent;
	ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
	static uint8_t whichCity;

  	switch ( CurrentState )
  	{
		case InitCityFrequency:
			CurrentState = CheckingCityFrequency;
			ES_Timer_InitTimer(CityFrequencyTimer, 2);
			break;
		case CheckingCityFrequency:
			if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == CityFrequencyTimer)) {
				whichCity = CheckCityPeriod();
				if ((whichCity != 16)){// && (LastValidCityPeriod != whichCity)) { //If city is not 16 then we found a valid city
					ES_Event ThisEvent;
					ThisEvent.EventType = ES_AtCity;
					ThisEvent.EventParam = whichCity;
					//printf("LastValidCityPeriod %d currentCityPeriod %d\r\n", LastValidCityPeriod, whichCity);
					PostCaptureCityService(ThisEvent); //Post to CaptureCityService to figure out if we need to capture
					PostMasterSM(ThisEvent); //Post to gameService to stop the motors and decide whether to capture or keep going
					LastValidCityPeriod = whichCity; // save our last valid period
				}
				ES_Timer_InitTimer(CityFrequencyTimer, 1); // restart the timer so that we keep checking city frequency
				
			} else if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == CityFrequencyOneShot)) {
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_NotAtCity;
				PostMasterSM(ThisEvent);
				printf("One shot timer for city freq\r\n");
				ValidCityPeriod = 16;// make city period invalid
				CandidateCityValue = 16;
			}
			break;
		}
		return ReturnEvent;
}
	
/****************************************************************************
 Function
    CheckCityPeriod

    Helper function to detect if there has been a valid city frequency found

 
****************************************************************************/
uint8_t CheckCityPeriod(void) {
	static uint8_t counter = 0;
	static uint32_t CurrentCityPeriod;
	static uint8_t LastCityValue = 16;
	static uint8_t NoCityValue = 16; //default to return city value 16 which indicates no city found
	CurrentCityPeriod = CityPeriod;
	for (uint8_t i = 0; i<16; i++) {
	if (((ValidCityFreqs[i]+FREQ_TOL) > CurrentCityPeriod) && ((ValidCityFreqs[i]-FREQ_TOL) < CurrentCityPeriod)) {
			CandidateCityValue = i; // Will be then a value from 0-15 
			break; //Should only find one frequency of city
		}
	}
	if (CandidateCityValue != LastCityValue) { //If current frequency reading is not the same as the last one then we got buggy value
		counter = 0;
	} else {
		counter ++;
	}
	if (counter > 4) {//We got 5 valid city frequencies in a row so take it as correct and return it
		counter = 0; //Reset our counter
		ValidCityPeriod = CandidateCityValue; //Save our last valid city frequency
		return CandidateCityValue; //Return a correct city value
	}
	LastCityValue = CandidateCityValue; //saved as static variable that we can call in another function
	return NoCityValue;
}


// ISR for city sensor
void InputCaptureResponseCityPeriod( void ){
	static uint32_t LastCapture;
	// start by clearing the source of the interrupt, the input capture event
	HWREG(WTIMER2_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
	
	ES_Timer_StopTimer(CityFrequencyOneShot);
	
	uint32_t ThisCapture;
	// now grab the captured value and calculate the period
	ThisCapture = HWREG(WTIMER2_BASE+TIMER_O_TAR);
	CityPeriod = ThisCapture - LastCapture;
	// update LastCapture to prepare for the next edge
	LastCapture = ThisCapture;
	ES_Timer_InitTimer(CityFrequencyOneShot, 200); //Making this time out at 200ms will tell us that there is no city
}

uint32_t GetValidCityPeriod (void){
	return ValidCityPeriod;
}