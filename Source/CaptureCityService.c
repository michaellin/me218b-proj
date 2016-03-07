/****************************************************************************
 Module
CaptureCityService.c

Description:
Flat state machine to capture city
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Driving.h"
#include "CaptureCityService.h"
#include "CommService.h"
#include "Master.h"
//#include "CommandCollectorSM.h"
#include "SendingCommSM.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include "driverlib/gpio.h"
#include "DetectCityFrequencyService.h" //for GetCityNumber function which may change location
#include "LEDService.h"

/*----------------------------- Module Defines ----------------------------*/
#define RED_Cmd 0
#define BLUE_Cmd 1
#define RS_COLOR_MASK 0x30
#define RS_ACK_Mask   0xc0
#define RS_LOC_Mask   0x0f
#define RED_Mask      0x20
#define BLUE_Mask     0x10
#define ALL_BITS   (0xff<<2)

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static uint8_t CommandParameter(uint8_t, uint8_t);
static void SendQuery( void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static CaptureCity_t CurrentState;
static uint8_t   MyColor = RED_Cmd;
static uint8_t Query = 0x70;
// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;
static uint8_t CityNumber; //number associated with city
static uint8_t RR; //response ready byte
static uint8_t RS; //response status byte

static uint8_t ThisCity;
static uint8_t MyColorMask;
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    InitCaptureCityService

****************************************************************************/
bool InitCaptureCityService ( uint8_t Priority )
{
  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = WaitingCapture;
  //Maybe get team color from somewhere instead of the define at the top 
  return true;
}

/****************************************************************************
 Function
     PostCaptureCityService

 
****************************************************************************/
bool PostCaptureCityService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunCaptureCityService

 
****************************************************************************/
ES_Event RunCaptureCityService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  if (ThisEvent.EventType == ES_SwitchTeam){
    if (MyColor == BLUE_Cmd){
      MyColor=RED_Cmd;
      SetRedLED();
      // Turn off all of the LEDs
      HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + (0xff<<2))) &= ~(GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
      // Turn on the new LEDs
      HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + (0xff<<2))) |= GPIO_PIN_1;
    } else {
      MyColor=BLUE_Cmd;
      SetBlueLED();
      // Turn off all of the LEDs
      HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + (0xff<<2))) &= ~(GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
      // Turn on the new LEDs
      HWREG(GPIO_PORTF_BASE+(GPIO_O_DATA + (0xff<<2))) |= GPIO_PIN_2;
    }
  }
  
  switch ( CurrentState )
  {
    case WaitingCapture :  //printf("WaitingCapture\n\r"); // If current state is initial Psedudo State
      if ( ThisEvent.EventType == ES_StartCaptureCity ) //Sent from game service or driving.c 
      { 
        //Get Frequency number of city
        CityNumber = ThisEvent.EventParam;
        printf("First City Frequency: %d \n\r",CityNumber);
        //Send Request Command with that frequency
        ES_Event ThisEvent;
        ThisEvent.EventType = ES_SPIWrite;
        ThisEvent.EventParam = CommandParameter(MyColor, CityNumber); //for command 2
        //printf("First City Parameter %x \n\r",ThisEvent.EventParam);
        PostCommSM(ThisEvent);
        // set next stae to RequestingChange1
        CurrentState = RequestingChange1;
        //Start timer to go to next state
        //ES_Timer_InitTimer(CaptureTimer,200);  
       }
     break;

    case RequestingChange1 :       // If current state is state one
      if (ThisEvent.EventType == ES_SPIWriteDone) 
      {
        //Start Timer
        ES_Timer_InitTimer(QueryTimer,15);
      }
      else if ( (ThisEvent.EventType == ES_TIMEOUT)&&(ThisEvent.EventParam == QueryTimer) )
      {
        //Send Query
        SendQuery();
        CurrentState = QueryingChange1;
      }
    break;
    
    case QueryingChange1 :
      //Read Response Ready  Byte
      RR = RR_Read();
      RS = RS_Read();
      if (ThisEvent.EventType == ES_SPIWriteDone) {
        //If Response is not ready then query again
        if (RR == 0x00){
          //Send Query 
          //Restart timer
          ES_Timer_InitTimer(QueryTimer,15);
        }
        else if (RR == 0xaa){
          //Go to next state ReadingFirstResponse
          CurrentState = ReadingFirstResponse;
          //Start timer to go to next state
          ES_Timer_InitTimer(CaptureTimer,3);
        }
        else { //check if error
          printf("Invalid RR value \n\r");
        }
      } else if ((ThisEvent.EventType == ES_TIMEOUT)&&(ThisEvent.EventParam == QueryTimer)) {
          SendQuery();
      }
    break;
    
    case ReadingFirstResponse :
      if ((ThisEvent.EventType == ES_TIMEOUT)&&(ThisEvent.EventParam == CaptureTimer)) 
      {
        //Check What RS is 
        if ((RS & RS_ACK_Mask) == 0x00){
        //Nack = wrong frequency!
        printf("Wrong Frequency! \n\r");
        //Go Back to waiting state and try again
        CurrentState = WaitingCapture;
        //Artifially tell the waiting state that you are at a city - May want to take this out and post to game service instead
        //              ThisEvent.EventType = ES_AtCity;
        //              PostCaptureCityService(ThisEvent);
        }
      //The ACK is what we're hoping to get
        else if ((RS & RS_ACK_Mask) == 0x40){
              //ACK = Correct frequency acknowledged
              printf("Correct Frequency!\r\n");
              ////////////////////////////////////////////////////////////////////////////////
              //I moved comparing if the city is our color to happen only after the second ack
              ////////////////////////////////////////////////////////////////////////////////
//              if (((MyColor==RED_Cmd)&&((RS&RS_COLOR_MASK)==RED_Mask))||((MyColor==BLUE_Cmd)&&((RS&RS_COLOR_MASK)==BLUE_Mask))){
//                //CurrentState = WaitingCapture;
//                ThisEvent.EventType = ES_IgnoreCity;
//                PostMasterSM(ThisEvent);
//                printf("Ignore City - Same Color \n\r");
//                //Go back to waiting state
//                CurrentState = WaitingCapture;
//              }  else {
//                printf("Dont Ignore City (Becuase its not our color)\r\n");
//              //Go to next state RequestingChange2
                CurrentState = RequestingChange2;
                //Start timer to go to next state
                ES_Timer_InitTimer(CaptureTimer,350); // allow several hundred milliseconds between packet transfers ***************** TWEAK THIS? ********************
              //} //end of else (that is commented out)
              //Read the rest of the Status Byte to tell what city we are at
              ThisCity = (RS & RS_LOC_Mask);
              printf("City: %x \n\r", ThisCity); 
      //        //Post to Navigation that we got a city name
      //        ES_Event ThisEvent;
      //        ThisEvent.EventType = ES_GotCityName;
      //        ThisEvent.EventParam = ThisCity;
      //        PostMasterSM(ThisEvent);
        }
        else if ((RS & RS_ACK_Mask) == 0x80){
          //Blocked = Under attack
          printf("We're Under Attack! \n\r");
          //Go Back to waiting state and try again
          CurrentState = WaitingCapture;
          //Artifially tell the waiting state that you are at a city - May want to take this out and post to game service instead
          //              ThisEvent.EventType = ES_AtCity;
          //              PostCaptureCityService(ThisEvent);
        }
        else if ((RS & RS_ACK_Mask) == 0xc0){
          //Busy = Someone else is going for this station also
          printf("Someone else is Here! \n\r");
          //Go Back to waiting state and try again
          CurrentState = WaitingCapture;
          //Artifially tell the waiting state that you are at a city - May want to take this out and post to game service instead
          //              ThisEvent.EventType = ES_AtCity;
          //              PostCaptureCityService(ThisEvent);
        }
        else{ //check if its a command you dont expect
          printf("Invalid RS value \n\r");
        }    
      }
      break;  

    case RequestingChange2 :       // If current state is state one
      if ( (ThisEvent.EventType == ES_TIMEOUT)&&(ThisEvent.EventParam == CaptureTimer)) 
      {
        //Get New Frequency number of city
        CityNumber = GetValidCityPeriod();
        printf("Second City Freq: %d \n\r",CityNumber);
        //Send Proper Frequency
        //Send Request Command with that frequency
        ES_Event ThisEvent;
        ThisEvent.EventType = ES_SPIWrite;
        ThisEvent.EventParam = CommandParameter(MyColor, CityNumber); //for command 2
        //printf("Command2Parameter %x \n\r",ThisEvent.EventParam);
        PostCommSM(ThisEvent);
        //Start Timer
        //ES_Timer_InitTimer(QueryTimer,15); CHANGED
        //FirstTime=1; CHANGED
      }
      //else if ( (ThisEvent.EventType == ES_TIMEOUT)&&(ThisEvent.EventParam == QueryTimer) ) CHANGED
      else if (ThisEvent.EventType == ES_SPIWriteDone)
      {      
        ES_Timer_InitTimer(QueryTimer,15);
        printf("Got second SPIWriteDone \r\n");
      } else if ((ThisEvent.EventType == ES_TIMEOUT)&&(ThisEvent.EventParam == QueryTimer) ) { // We only transition after we have done the first Query
        CurrentState = QueryingChange2;
        SendQuery();
      }
      break;
    case QueryingChange2 :
      if (ThisEvent.EventType == ES_SPIWriteDone)
      {
        //Read Response Ready  Byte
        RR = RR_Read();
        RS = RS_Read();
        //If Response is not ready then query again
        if (RR == 0x00){
          //Restart timer
          ES_Timer_InitTimer(QueryTimer,15);
          //printf("Query not ready\r\n");
        }
        else if (RR == 0xaa){
          //Go to next state ReadingFirstResponse
          CurrentState = ReadingSecondResponse;
          //printf("Got RS 0x%x\r\n", RS_Read());
          //Start timer to go to next state
          ES_Timer_InitTimer(CaptureTimer,3);
        }
        else { //check if error
          printf("Invalid RR value \n\r");
        }
      } else if ((ThisEvent.EventType == ES_TIMEOUT)&&(ThisEvent.EventParam == QueryTimer) ) {
          //Send Query 
          SendQuery();
      }
      break;
    case ReadingSecondResponse :
      if ( (ThisEvent.EventType == ES_TIMEOUT)&&(ThisEvent.EventParam == CaptureTimer)) 
      {
        //Check What RS is 
        if ((RS & RS_ACK_Mask) == 0x00){
          //Nack = wrong frequency!
          printf("Wrong Frequency2! \n\r");
          //Go Back to waiting state and try again
          CurrentState = WaitingCapture;
        }
        //The ACK is what we're hoping to get
        else if ((RS & RS_ACK_Mask) == 0x40){
          //ACK = Correct frequency acknowledged
          printf("Acknowledged!");
          if ((RS & RS_COLOR_MASK)==0x00){
            printf(" ... but unclaimed\r\n");
          } else if ((RS & RS_COLOR_MASK)==0x10){
            printf(" ... and city converted to blue\r\n");
          } else if ((RS & RS_COLOR_MASK)==0x20){
            printf(" ... and city converted to red\r\n");
          } else if ((RS & RS_COLOR_MASK)==0x30){
            printf(" ... and city is undefined?!\r\n");                  
          }
          
          //Moved from first ACK: Check if the city you are trying to capture is the same color as you
          if (((MyColor==RED_Cmd)&&((RS&RS_COLOR_MASK)==RED_Mask))||((MyColor==BLUE_Cmd)&&((RS&RS_COLOR_MASK)==BLUE_Mask))){
                printf("City is same Color as you so move on  (2nd Ack) \n\r");
              }  else {
                printf("City should be converted to your color!\r\n");
              }
          
          
          //Read the rest of the Status Byte to tell what city we are at
          ThisCity = (RS & RS_LOC_Mask);
          printf("City: %x \n\r", ThisCity); 
          //Post to Navigation that we got a city name
          ES_Event ThisEvent;
          ThisEvent.EventType = ES_GotCityName;
          ThisEvent.EventParam = ThisCity;
          PostMasterSM(ThisEvent);
          
          if (MyColor == RED_Cmd){
              MyColorMask = 0x20;
          }
          else if (MyColor == BLUE_Cmd){
              MyColorMask = 0x10;
          }
          
          //Post to GameService to ignore city
          if ((RS & RS_COLOR_MASK)==MyColorMask){ // If it is our color
            if (~(((ThisCity == 0x02) &&(MyColor == BLUE_Cmd)) || ((ThisCity == 0x07)&&(MyColor == RED_Cmd)))){
              ThisEvent.EventType = ES_IgnoreCity;
              PostMasterSM(ThisEvent);
              //Go to captured state
              CurrentState  = CapturedState;
            }
          }
          else {
            CurrentState  = WaitingCapture; //If it remained unclaimed we should try to capture again and not move on
          }
        }
        else if ((RS & RS_ACK_Mask) == 0x80){
          //Blocked = Under attack
          printf("We're Under Attack2! \n\r");
          //Go Back to waiting state and try again
          CurrentState = WaitingCapture;
          //Artifially tell the waiting state that you are at a city - May want to take this out and post to game service instead
          //              ThisEvent.EventType = ES_AtCity;
          //              PostCaptureCityService(ThisEvent);
        }
        else if ((RS & RS_ACK_Mask) == 0xc0){
          //Busy = Someone else is going for this station also
          printf("Someone else is Here2! \n\r");
          //Go Back to waiting state and try again
          CurrentState = WaitingCapture;
          //Artifially tell the waiting state that you are at a city - May want to take this out and post to game service instead
          //              ThisEvent.EventType = ES_AtCity;
          //              PostCaptureCityService(ThisEvent);
        }
        else{ //check if its a command you dont expect
          printf("Invalid RS value2 \n\r");
        }
      }
      break;  
      
      case CapturedState :
      if ( ThisEvent.EventType == ES_OkToWaitCapture) 
      {
        //Go back to WaitingCapture State after we switch to ignore city state
        //Sent from game service when it finishes transitioning to ignore city
        CurrentState = WaitingCapture;
      }
      break;
      
  }                                   // end switch 
  return ReturnEvent;
}

//Module level Functions 
//Helper function to figure out what command paramter to send to CommService
static uint8_t CommandParameter(uint8_t Color, uint8_t FreqCityNumber){
  uint8_t ReturnVal;
//  uint8_t RequestedColor;
  if (Color == RED_Cmd){
    printf("Requesting RED Cmd\r\n");
  }
  else if (Color == BLUE_Cmd) {
    printf("Requesting Blue Cmd\r\n");
  }
  
//  if (Color != RequestedColor) {
//    printf("We are trying to request a color that is not our color. Not good.\r\n");
//  }
  //printf("FreqCityNumber: %d\n\r", FreqCityNumber);
  ReturnVal = 0x80 + (Color<<5) + (Color<<4) + FreqCityNumber;//0x80 + (Color<<5) +(RequestedColor<<4) + FreqCityNumber; 
  //printf("ReturnVal: %x\n\r", ReturnVal);
  return ReturnVal;
}

//Send Query Function
static void SendQuery( void){
  ES_Event ThisEvent;
  ThisEvent.EventType = ES_SPIWrite;
  ThisEvent.EventParam = Query; //Query=0x70
  PostCommSM(ThisEvent);
}

uint8_t OurTeamColor(void){
  return MyColor;
}
