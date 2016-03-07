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
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"

#include "ButtonDebounce.h"
#include "Master.h"
#include "CaptureCityService.h"

// All_BITS
#define ALL_BITS (0xff<<2)

// Data Private to Module
static uint8_t LastButtonState;
static uint8_t MyPriority;
static ButtonState_t CurrentState;

// Function Definitions
bool InitButtonDB (uint8_t Priority){
  MyPriority = Priority;
  
  //StartButton (Dig Input) ---> F4
  // Initialize port F
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R5 ;
  while((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R5) != SYSCTL_PRGPIO_R5);
  // Initialize bit 4
  HWREG(GPIO_PORTF_BASE+GPIO_O_DEN) |= (GPIO_PIN_4);
  HWREG(GPIO_PORTF_BASE+GPIO_O_PUR) |= (BIT4HI);
  
  
  // Sample the button port pin
  LastButtonState = (HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA+ALL_BITS)) & BIT4HI);
  // Set CurrentState to be Debouncing
  CurrentState = Debouncing;
  // Start debounce timer
  ES_Timer_InitTimer(DB_TIMER, 30); // Timer0, debounce time of 30 ms
  return true;
}

bool PostButton( ES_Event ThisEvent ){
  return ES_PostToService( MyPriority, ThisEvent);
}

bool CheckButtonEvents(void){
  // Local Variables
  ES_Event ThisEvent;
  uint8_t ReturnVal = false;
  // Set CurrentButtonState to state read from port pin
  uint8_t CurrentButtonState = (HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA+ALL_BITS)) & BIT4HI);
  // If the CurrentButtonState is different from the LastButtonState
  if (CurrentButtonState != LastButtonState){
    ReturnVal = true;
    // If the CurrentButtonState is down
    if (CurrentButtonState == BIT4HI) {
      ThisEvent.EventType = ES_BUTTON_DOWN;
      PostButton(ThisEvent);
    }
    else{ // button is up
      ThisEvent.EventType = ES_BUTTON_UP;
      PostButton(ThisEvent);
    }
  }
  LastButtonState = CurrentButtonState;
  return ReturnVal;
}

ES_Event RunButtonDB( ES_Event ThisEvent ){
  // If CurrentState is Debouncing
  if (CurrentState == Debouncing){
    // If EventType is ES_TIMEOUT & parameter is debounce timer number
    if ( (ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == DB_TIMER) ){ // 0 is debounce timer number
      CurrentState = Ready2Sample;
    }
  }
  // Else if CurrentState is Ready2Sample
  else if (CurrentState == Ready2Sample){
    // If EventType is ButtonUp
    if (ThisEvent.EventType == ES_BUTTON_UP){
      // Start debounce timer
      ES_Timer_InitTimer(DB_TIMER,30);
      CurrentState = Debouncing;
      // Post DB_button_up event?
    }
    // If EventType is ButtonDown
    else if (ThisEvent.EventType == ES_BUTTON_DOWN){
      // Start debounce timer
      ES_Timer_InitTimer(DB_TIMER,30);
      CurrentState = Debouncing;
      // Post DBButtonDown
      ES_Event ThisEvent;
      ThisEvent.EventType = ES_SwitchTeam;
      PostCaptureCityService(ThisEvent);
    }
  }
  ThisEvent.EventType = ES_NO_EVENT;
  return ThisEvent;
}
