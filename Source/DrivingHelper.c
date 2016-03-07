/****************************************************************************
 Module
   DrivingHelper.c

 Revision
   2.0.1

 Description
   This module has all helper functions that interact with the motor drivers

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/14/16 12:31 mal      Init coding
****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_pwm.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"	// Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"
#include "BITDEFS.h"
#include "DrivingHelper.h"


/*---------------------------- Module Prototypes ---------------------------*/
void SetDutyLeft(int Duty);
void SetDutyRight(int Duty);
static uint8_t LinearMapDuty(uint8_t duty);


/*---------------------------- Macro Defines ---------------------------*/
#define ALL_BITS (0xff<<2) 

/****************************************************************************
 Function
     SetDutyLeft

 Parameters
     int requested duty

 Returns
     None

 Description
     Sets the duty cycle for PB5 on Forward (positive) or Backward (negative) drive mode
 Notes


 Author
     Team 13, 2/21/05, 8:04PM
****************************************************************************/
void SetDutyLeft(int Duty){
	//Check if requested duty cycle is for forwards or backwards and set the corresponding registers
	uint8_t absDuty = (uint8_t)((Duty > 0)?Duty:-Duty);
	uint8_t MappedDuty = LinearMapDuty(absDuty);
	//Decide the direction
	if (Duty >= 0) {
		//make sure PWM is not inverted on PWM3
		HWREG( PWM0_BASE+PWM_O_INVERT) &= ~(BIT3HI);
		//Set PB4 low
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(BIT4HI);
	} else if (Duty < 0) {
		//make sure PWM is inverted on PWM3
		HWREG( PWM0_BASE+PWM_O_INVERT) |= (BIT3HI);
		//Set PB4 high
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT4HI);
	}
	
	if (MappedDuty == 100) {
		// To program 100% DC, simply set the action on Zero to set the output to one
		HWREG( PWM0_BASE+PWM_O_1_GENB) = PWM_1_GENB_ACTZERO_ONE;
	}	else if (MappedDuty == 0) {
		// To program 0% DC, simply set the action on Zero to set the output to zero
		HWREG( PWM0_BASE+PWM_O_1_GENB) = PWM_1_GENB_ACTZERO_ZERO;
	} else {
		// restore the proper actions when the DC drops below 100%
		// or rises above 0% 
		// program generator B to go to 1 at rising compare A, 0 on falling compare A
		HWREG( PWM0_BASE+PWM_O_1_GENB) =
			(PWM_0_GENB_ACTCMPBU_ONE | PWM_1_GENB_ACTCMPBD_ZERO );
		// Set the Duty cycle on B to Duty by programming the compare value
		// to 100 minus that percent of the value in the Load register, which is half 
		// the total period.
		HWREG( PWM0_BASE+PWM_O_1_CMPB) = HWREG( PWM0_BASE+PWM_O_1_LOAD) * (100 - MappedDuty) / 100;
	}
}

/****************************************************************************
 Function
     SetDutyRight

 Parameters
     int requested duty

 Returns
     None

 Description
     Sets the duty cycle for PB6 on Forward (positive) or Backward (negative) drive mode
 Notes


 Author
     Team 13, 2/21/05, 8:04PM
****************************************************************************/
void SetDutyRight(int Duty){
	//Check if requested duty cycle is for forwards or backwards and set the corresponding registers
	uint8_t absDuty = (uint8_t)((Duty > 0)?Duty:-Duty);
	uint8_t MappedDuty = LinearMapDuty(absDuty);
	//Decide the direction
	if (Duty >= 0) {
			//make sure PWM is not inverted on PWM0
		HWREG( PWM0_BASE+PWM_O_INVERT) &= ~(BIT0HI);
		//Set PB6 low
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(BIT7HI);
	} else if (Duty < 0) {
		//make sure PWM is inverted on PWM0
		HWREG( PWM0_BASE+PWM_O_INVERT) |= (BIT0HI);
		//Set PB6 high
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BIT7HI);
	}
	
	if (MappedDuty == 100) {
		// To program 100% DC, simply set the action on Zero to set the output to one
		HWREG( PWM0_BASE+PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ONE;
	}
	else if (MappedDuty == 0) {
		// To program 0% DC, simply set the action on Zero to set the output to zero
		HWREG( PWM0_BASE+PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ZERO;
	} else {
		// restore the proper actions when the DC drops below 100%
		// or rises above 0% 
		// program generator A to go to 1 at rising compare A, 0 on falling compare A
		HWREG( PWM0_BASE+PWM_O_0_GENA) =
			(PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO );
		// Set the Duty cycle on A to Duty by programming the compare value
		// to 100 minus that percent of the value in the Load register, which is half 
		// the total period.
		HWREG( PWM0_BASE+PWM_O_0_CMPA) = HWREG( PWM0_BASE+PWM_O_0_LOAD) * (100 - MappedDuty) / 100;
	}
}


/****************************************************************************
 Function
     LinearMapDuty

 Parameters
     int requested duty

 Returns
     int linearized duty

 Description
     Scales and offsets the duty cycle to be in a linear region (10% to 100%)
 Notes


 Author
     Team 13, 2/21/05, 8:04PM
****************************************************************************/
static uint8_t LinearMapDuty(uint8_t duty) {
	return (90*duty)/100+10; // TODO right now mapping to 99 duty cycle fix 100% edge case when have time
}
