/*----------------------------- Include Files -----------------------------*/
/* include header files for the framework and this service
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"	// Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"
#include "inc/hw_timer.h"

#include "LEDService.h"
#include "InitHWModules.h"

// Defines
#define ALL_BITS (0xff<<2)

#define AllColors 0
#define Red 1
#define Green 2
#define Blue 3
#define Teal 4
#define Purple 5
#define Orange 6

// Pin Assignments
// Red - E2
// Green - E3
// Blue - B2

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint8_t ColorArray[6] = {Red,Orange,Green,Teal,Blue,Purple};
static uint8_t i;

// Init function
bool InitLEDService ( uint8_t Priority )
{
  ES_Event ThisEvent;
  MyPriority = Priority;
	InitLEDStrip();
//  if (ES_PostToService( MyPriority, ThisEvent) == true)
//  {  
//    return true;
//  }else
//  {
//      return false;
//  }
	return true;
}

//Post function
bool PostLEDService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

// Run Function 
ES_Event RunLEDService( ES_Event ThisEvent )
{ 
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  
	if (ThisEvent.EventType == ES_Celebrate){ 
		ES_Timer_InitTimer(CELEB_TIMER, 150);
	} else if ((ThisEvent.EventType == ES_TIMEOUT)&&(ThisEvent.EventParam == CELEB_TIMER)) {
		if (i<5){ //printf("here");
			SetLEDLow(AllColors);
			SetLEDHigh(ColorArray[i]);
			ES_Timer_InitTimer(CELEB_TIMER , 150);
			i++;
		} else if (i>=5){
			SetLEDLow(AllColors);
			SetLEDHigh(ColorArray[i]);
			ES_Timer_InitTimer(CELEB_TIMER , 50);
			i=0;
		}
	}	else if (ThisEvent.EventType == ES_StopCelebrate) {
//		
			ES_Timer_StopTimer(CELEB_TIMER);
	}
  return ReturnEvent;
}

// Public Helper/Getter Functions

void InitLEDStrip(void){
	volatile uint32_t Dummy;
	// enable the clock to Port E
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R4;
	// enable clock to Port B
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;
	// kill a few cycles to let the peripheral clock get going
	Dummy = HWREG(SYSCTL_RCGCGPIO);
	//SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	
	// Enable pins for digital I/O
	HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |= (GPIO_PIN_2 | GPIO_PIN_3);
	HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (GPIO_PIN_2);
	// make pins E2,E3, and B2 into outputs
	HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) |= (GPIO_PIN_2 | GPIO_PIN_3);
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (GPIO_PIN_2);
	
//	//Set LED strip low
//	SetLEDLow(AllColors);
	HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) &= (BIT2LO);
	HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) &= (BIT3LO);
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= (BIT2LO);
}

void SetLEDHigh(uint8_t Color){
	if (Color == AllColors){
		// Set all pins high
		HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT2HI | BIT3HI);
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT2HI);
	} else if (Color == Red){
		// Set E2 high
		HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT2HI);
	} else if (Color == Green){
		// Set E3 high
		HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT3HI);
	} else if (Color == Blue) {
		// Set B2 high
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT2HI);
	} else if (Color == Teal){
		// Set E3 and B2 high
		HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT3HI);
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT2HI);
	} else if (Color == Purple){
		// Set E2 and B2 high
		HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT2HI);
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT2HI);
	} else if (Color == Orange){
		// Set E2 and E3 high 
		HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT2HI | BIT3HI);
	}
}

void SetLEDLow(uint8_t Color){
	if (Color == AllColors){
		// Set all pins low
		HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(BIT2HI | BIT3HI);
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= (BIT2LO);
	} else {
		if (Color == Red){
			// Set E2 low
			HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) &= (BIT2LO);
		} else if (Color == Green){
			// Set E3 low
			HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)) &= (BIT3LO);
		} else if (Color == Blue){ 
			// Set B2 low
			HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= (BIT2LO);
		}
	}
}

void SetRedLED( void){
	SetLEDLow(AllColors);
	SetLEDHigh(Red);
}

void SetBlueLED( void){
	SetLEDLow(AllColors);
	SetLEDHigh(Blue);
}
