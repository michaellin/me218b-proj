/****************************************************************************
 Module
ShootingService.c

Description:
Flat state machine to Shoot Ball
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Driving.h"
#include "Master.h"
#include "ShootingService.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"


/*----------------------------- Module Defines ----------------------------*/
#define ShootingServoDown 3
#define ShootingServoUp 8
#define CServoDown 9
#define CServoUp 5
#define CServoMiddle 7
#define CTime 300
#define ShootingTime 2000



/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static void SetDutyCServo(uint8_t Duty);
static void SetDutyShootingServo(uint8_t Duty);
/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;
static ShootingService_t CurrentState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    InitShootingService

****************************************************************************/
bool InitShootingService ( uint8_t Priority )
{
  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = Waiting2ShootState;
  //Maybe get team color from somewhere instead of the define at the top 
  SetDutyCServo(CServoMiddle);
  SetDutyShootingServo(ShootingServoUp);
  return true;
}

/****************************************************************************
 Function
     PostCaptureCityService

 
****************************************************************************/
bool PostShootingService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunCaptureCityService

 
****************************************************************************/
ES_Event RunShootingService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch ( CurrentState )
  {
    case Waiting2ShootState :  
        if ( ThisEvent.EventType == ES_StartShooting ) //Sent from game service or driving.c 
        { 
            SetDutyShootingServo(ShootingServoDown); //Make this function later to move servo down
            CurrentState = ShootingServoDownState;
            //Start timer to go to next state and let servo lower
            ES_Timer_InitTimer(ShootingTimer,ShootingTime);  
         }
     break;
         
     case ShootingServoDownState :  
        if ( (ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == ShootingTimer)) //Sent from game service or driving.c 
        { 
            SetDutyShootingServo(ShootingServoUp); //Make this function later to move servo up
            CurrentState = ShootingServoUpState;
            //Start timer to go to next state and let servo lower
            ES_Timer_InitTimer(ShootingTimer,ShootingTime);  
         }
     break;

     case ShootingServoUpState :  
        if ( (ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == ShootingTimer)) //Sent from game service or driving.c 
        { 
            SetDutyCServo(CServoDown);
            CurrentState = CServoDownState;
            //Start timer to go to next state and let servo lower
            ES_Timer_InitTimer(ShootingTimer,CTime);  
         }
     break;

     case CServoDownState :  
        if ( (ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == ShootingTimer)) //Sent from game service or driving.c 
        { 
            SetDutyCServo(CServoUp);
            CurrentState = CServoUpState;
            //Start timer to go to next state and let servo lower
            ES_Timer_InitTimer(ShootingTimer,CTime);  
         }
     break;
         
     case CServoUpState :  
        if ( (ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == ShootingTimer)) //Sent from game service or driving.c 
        { 
            SetDutyCServo(CServoMiddle); //Make this function later to move servo down
            CurrentState = CServoMiddleState;
            //Start timer to go to next state and let servo lower
            ES_Timer_InitTimer(ShootingTimer,CTime);  
         }
     break;
         
     case CServoMiddleState :  printf("CServoMiddleState!!!!!!!!!!!!!\r\n");
        if ( (ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == ShootingTimer)) //Sent from game service or driving.c 
        { 
            CurrentState = Waiting2ShootState;
            //Post ShotBall to NavigationService
            ES_Event ThisEvent;
            ThisEvent.EventType = ES_BallShot;
            printf("ES_Ball Shot sent!!!\n\r");
            PostMasterSM(ThisEvent);
         }
     break;
    
  }                                   // end switch 
  return ReturnEvent;
}

static void SetDutyCServo(uint8_t Duty){ //Pin E4 (Module 0, Generator 2, Channel A)
  if (Duty == 100) {
    // To program 100% DC, simply set the action on Zero to set the output to one
    HWREG( PWM0_BASE+PWM_O_2_GENA) = PWM_2_GENA_ACTZERO_ONE;
  }
  else if (Duty == 0) {
    // To program 0% DC, simply set the action on Zero to set the output to zero
    HWREG( PWM0_BASE+PWM_O_2_GENA) = PWM_2_GENA_ACTZERO_ZERO;
  }
  else {
    // restore the proper actions when the DC drops below 100%
    // or rises above 0% 
    // program generator A to go to 1 at rising compare A, 0 on falling compare A
    HWREG( PWM0_BASE+PWM_O_2_GENA) =
      (PWM_2_GENA_ACTCMPAU_ONE | PWM_2_GENA_ACTCMPAD_ZERO );
    // Set the Duty cycle on A to Duty by programming the compare value
    // to 100 minus that percent of the value in the Load register, which is half 
    // the total period.
    HWREG( PWM0_BASE+PWM_O_2_CMPA) = HWREG( PWM0_BASE+PWM_O_2_LOAD) * (100 - Duty) / 100;
    
  }
}

static void SetDutyShootingServo(uint8_t Duty){ //Pin E5 (Module 0, Generator 2, Channel B)
  if (Duty == 100) {
    // To program 100% DC, simply set the action on Zero to set the output to one
    HWREG( PWM0_BASE+PWM_O_2_GENB) = PWM_2_GENB_ACTZERO_ONE;
  }
  else if (Duty == 0) {
    // To program 0% DC, simply set the action on Zero to set the output to zero
    HWREG( PWM0_BASE+PWM_O_2_GENB) = PWM_2_GENB_ACTZERO_ZERO;
  }
  else {
    // restore the proper actions when the DC drops below 100%
    // or rises above 0% 
    // program generator B to go to 1 at rising compare B, 0 on falling compare B
    HWREG( PWM0_BASE+PWM_O_2_GENB) =
      (PWM_2_GENB_ACTCMPBU_ONE | PWM_2_GENB_ACTCMPBD_ZERO );
    // Set the Duty cycle on B to Duty by programming the compare value
    // to 100 minus that percent of the value in the Load register, which is half 
    // the total period.
    HWREG( PWM0_BASE+PWM_O_2_CMPB) = HWREG( PWM0_BASE+PWM_O_2_LOAD) * (100 - Duty) / 100;
    
  }
}

