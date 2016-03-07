/****************************************************************************
 Module
   InitModules.c

 Revision
   2.0.1

 Description
   This module has all helper functions to initialize the necessary Tiva hardware.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/14/16 12:31 mal      Init coding
****************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
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
#include "BITDEFS.H"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"

/*---------------------------- Module Variables ---------------------------*/

/*---------------------------- Module Defines ---------------------------*/
#define TicksPerMs          40000 // 40k ticks per ms
#define PWMTicksPerMS       1250/5 //40,000/32
#define PeriodInMS          1
#define ServoPeriod					20
#define ServoTicksPerMS     1250  //40,000/32
#define BitsPerNibble       4
#define ALL_BITS           (0xff<<2)

static uint16_t TicksPerMS = 40000;

/****************************************************************************
 Function
     InitPWMLeft

 Parameters
     None

 Returns
     None

 Description
     Initializes PWM module 0 with PB5
 Notes

 Author
     Team 13, 2/11/05, 10:38AM
****************************************************************************/
void InitPWMLeft( void ){
	volatile uint32_t Dummy; // use volatile to avoid over-optimization
// start by enabling the clock to the PWM Module (PWM0)
	HWREG(SYSCTL_RCGCPWM) |= SYSCTL_RCGCPWM_R0;
// enable the clock to Port B
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;
// Select the PWM clock as System Clock/32
	HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
		(SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_32);
// make sure that the PWM module clock has gotten going
	while ((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
		;
// disable the PWM while initializing
	HWREG( PWM0_BASE+PWM_O_1_CTL ) &= ~PWM_1_CTL_ENABLE;
// program generator B to go to 1 at rising compare A, 0 on falling compare A
	HWREG( PWM0_BASE+PWM_O_1_GENB) =
		(PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO );
// program generator A to always be 0 by setting the action on Zero to set
// the output to zero
	//HWREG( PWM0_BASE+PWM_O_1_GENA) = PWM_1_GENA_ACTZERO_ZERO;
// Set the PWM period. Since we are counting both up & down, we initialize
// the load register to 1/2 the desired total period. We will also program
// the match compare registers to 1/2 the desired high time
	HWREG( PWM0_BASE+PWM_O_1_LOAD) = ((PeriodInMS * PWMTicksPerMS)-1)>>1;
// enable the PWM outputs
	HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM3EN);
// now configure the Port B pins to be PWM outputs
// start by selecting the alternate function for PB5
	HWREG(GPIO_PORTB_BASE+GPIO_O_AFSEL) |= (BIT5HI);
// now choose to map PWM to those pins, this is a max value of 4 that we
// want to use for specifying the function on bits 5
	HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) =
		(HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) & 0xff0ffff) + 
		(4<<(5*BitsPerNibble));
// Enable pins 5 on Port B for digital I/O
	HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT5HI);
// make pins 5 on Port B into outputs
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT5HI);
// set the up/down count mode, enable the PWM generator and make
// both generator updates locally synchronized to zero count
	HWREG(PWM0_BASE+ PWM_O_1_CTL) = (PWM_1_CTL_MODE | PWM_1_CTL_ENABLE |
		PWM_1_CTL_GENBUPD_LS);
}


/****************************************************************************
 Function
     InitPWMRight

 Parameters
     None

 Returns
     None

 Description
     Initializes PWM module 0 with PB6.


 Author
     Team 13, 2/11/05, 10:38AM
****************************************************************************/
void InitPWMRight( void ){																	
	volatile uint32_t Dummy; // use volatile to avoid over-optimization
// start by enabling the clock to the PWM Module (PWM0)
	HWREG(SYSCTL_RCGCPWM) |= SYSCTL_RCGCPWM_R0;
// enable the clock to Port B
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;
// Select the PWM clock as System Clock/32
	HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
		(SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_32);
// make sure that the PWM module clock has gotten going
	while ((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
		;
// disable the PWM while initializing
	HWREG( PWM0_BASE+PWM_O_0_CTL ) = 0;
// program generator B to go to 1 at rising compare B, 0 on falling compare B
	HWREG( PWM0_BASE+PWM_O_0_GENA) =
		(PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO );
// program generator A to always be 0 by setting the action on Zero to set
// the output to zero
	//HWREG( PWM0_BASE+PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ZERO;
// Set the PWM period. Since we are counting both up & down, we initialize
// the load register to 1/2 the desired total period. We will also program
// the match compare registers to 1/2 the desired high time
	HWREG( PWM0_BASE+PWM_O_0_LOAD) = ((PeriodInMS * PWMTicksPerMS)-1)>>1;
// enable the PWM outputs
	HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM0EN);
// now configure the Port B pins to be PWM outputs
// start by selecting the alternate function for PB6
	HWREG(GPIO_PORTB_BASE+GPIO_O_AFSEL) |= (BIT6HI);
// now choose to map PWM to those pins, this is a max value of 4 that we
// want to use for specifying the function on bit 6
	HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) =
		(HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) & 0xf0ffffff) + (4<<(6*BitsPerNibble));
// Enable pin 6 on Port B for digital I/O
	HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT6HI);
// make pin 6 on Port B into outputs
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT6HI);
// set the up/down count mode, enable the PWM generator and make
// both generator updates locally synchronized to zero count
	HWREG(PWM0_BASE+ PWM_O_0_CTL) = (PWM_0_CTL_MODE | PWM_0_CTL_ENABLE |
		 PWM_0_CTL_GENAUPD_LS);
}


void InitMotorLines( void){
	//Enable GPIO Port B, 
	HWREG(SYSCTL_RCGCGPIO) |= (BIT1HI);
	//Wait a few cycles for clock
	while ((HWREG(SYSCTL_PRGPIO) & BIT1HI) != BIT1HI)
		;
	//Assign Digital Port 
	HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (GPIO_PIN_4 |GPIO_PIN_7);

	//Assign Port B pin PB4, PB7 (output)
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (GPIO_PIN_4 | GPIO_PIN_7);
		
	//Initialize PB4 and PB7 low
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~ (GPIO_PIN_4);
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~ (GPIO_PIN_7);
}


/****************************************************************************
 Function
     InitLED

 Parameters
     None

 Returns
     None

 Description
     Initializes TIVA for blue LED for debugging.

 Author
     Team 13, 2/23/16, 10:38AM
****************************************************************************/
void InitLED(void)
{
	volatile uint32_t Dummy;
	// enable the clock to Port F
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R5;
	// kill a few cycles to let the peripheral clock get going
	Dummy = HWREG(SYSCTL_RCGCGPIO);
	//SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	
	// Enable pins for digital I/O
	HWREG(GPIO_PORTF_BASE+GPIO_O_DEN) |= (GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	
	// make pins 1,2 & 3 on Port F into outputs
	HWREG(GPIO_PORTF_BASE+GPIO_O_DIR) |= (GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	//GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
}

//Encoder for motor 1 is in pin D2; We are using Wide Timer 3 A
void InitInputCapturePeriodLeft( void ){
// start by enabling the clock to the timer (Wide Timer 3)
	HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R3;
	while ((HWREG(SYSCTL_RCGCWTIMER) & SYSCTL_PRWTIMER_R3) != SYSCTL_PRWTIMER_R3)
		;
// enable the clock to Port D
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;
// since we added this Port D clock init, we can immediately start
// into configuring the timer, no need for further delay
// make sure that timer (Timer A) is disabled before configuring
	HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
// set it up in 32bit wide (individual, not concatenated) mode
// the constant name derives from the 16/32 bit timer, but this is a 32/64
// bit timer so we are setting the 32bit mode
	HWREG(WTIMER3_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
// we want to use the full 32 bit count, so initialize the Interval Load
// register to 0xffff.ffff (its default value :-)
	HWREG(WTIMER3_BASE+TIMER_O_TAILR) = 0xffffffff;
// set up timer A in capture mode (TAMR=3, TAAMS = 0),
// for edge time (TACMR = 1) and up-counting (TACDIR = 1)
	HWREG(WTIMER3_BASE+TIMER_O_TAMR) =
		(HWREG(WTIMER3_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) |
		(TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
// To set the event to rising edge, we need to modify the TAEVENT bits
// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
	HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
// Now Set up the port to do the capture (clock was enabled earlier)
// start by setting the alternate function for Port D bit 2 (WT3CCP0)
	HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= BIT2HI;
// Then, map bit 2's alternate function to WT3CCP0
// 7 is the mux value to select WT3CCP0, 16 to shift it over to the
// right nibble for bit 4 (4 bits/nibble * 4 bits)
	HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) =
		(HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xfffff0ff) + (7<<8);
// Enable pin on Port D for digital I/O
	HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= BIT2HI;
// make pin 2 on Port D into an input
	HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= BIT2LO;
// back to the timer to enable a local capture interrupt
	HWREG(WTIMER3_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
// enable the Timer A in Wide Timer 0 interrupt in the NVIC
// it is interrupt number 100 so appears in EN3 at bit 4
	HWREG(NVIC_EN3) |= BIT4HI;
// make sure interrupts are enabled globally
	__enable_irq();
// now kick the timer off by enabling it and enabling the timer to
// stall while stopped by the debugger
	HWREG(WTIMER3_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
}

//Encoder for motor 1 is in pin C6; We are using Wide Timer 1 A
void InitInputCapturePeriodRight( void ){
// start by enabling the clock to the timer (Wide Timer 1)
	HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R1;
	while ((HWREG(SYSCTL_RCGCWTIMER) & SYSCTL_PRWTIMER_R1) != SYSCTL_PRWTIMER_R1)
		;
// enable the clock to Port C
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R2;
// since we added this Port C clock init, we can immediately start
// into configuring the timer, no need for further delay
// make sure that timer (Timer A) is disabled before configuring
	HWREG(WTIMER1_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
// set it up in 32bit wide (individual, not concatenated) mode
// the constant name derives from the 16/32 bit timer, but this is a 32/64
// bit timer so we are setting the 32bit mode
	HWREG(WTIMER1_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
// we want to use the full 32 bit count, so initialize the Interval Load
// register to 0xffff.ffff (its default value :-)
	HWREG(WTIMER1_BASE+TIMER_O_TAILR) = 0xffffffff;
// set up timer A in capture mode (TAMR=3, TAAMS = 0),
// for edge time (TACMR = 1) and up-counting (TACDIR = 1)
	HWREG(WTIMER1_BASE+TIMER_O_TAMR) =
		(HWREG(WTIMER1_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) |
		(TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
// To set the event to rising edge, we need to modify the TAEVENT bits
// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
	HWREG(WTIMER1_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
// Now Set up the port to do the capture (clock was enabled earlier)
// start by setting the alternate function for Port C bit 6 (WT0CCP0)
	HWREG(GPIO_PORTC_BASE+GPIO_O_AFSEL) |= BIT6HI;
// Then, map bit 5's alternate function to WT0CCP0
// 7 is the mux value to select WT0CCP0, 24 to shift it over to the
// right nibble for bit 6 (4 bits/nibble * 6 bits)
	HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) =
		(HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) & 0xf0ffffff) + (7<<24);
// Enable pin on Port C for digital I/O
	HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= BIT6HI;
// make pin 6 on Port C into an input
	HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) &= BIT6LO;
// back to the timer to enable a local capture interrupt
	HWREG(WTIMER1_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
// enable the Timer A in Wide Timer 1 interrupt in the NVIC
// it is interrupt number 96 so appears in EN3 at bit 0
	HWREG(NVIC_EN3) |= BIT0HI;
// make sure interrupts are enabled globally
	__enable_irq();
// now kick the timer off by enabling it and enabling the timer to
// stall while stopped by the debugger
	HWREG(WTIMER1_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
}


// Uses Wide Timer 0 B for motor PID updates
void InitMotorVelocityPID( void ){
// start by enabling the clock to the timer (Wide Timer 0)
	HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R0; // kill a few cycles to let the clock get going
	while ((HWREG(SYSCTL_RCGCWTIMER) & SYSCTL_PRWTIMER_R0) != SYSCTL_PRWTIMER_R0)
		;
// make sure that timer (Timer B) is disabled before configuring
	HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
// set it up in 32bit wide (individual, not concatenated) mode
	HWREG(WTIMER0_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
// set up timer B in periodic mode so that it repeats the time-outs
	HWREG(WTIMER0_BASE+TIMER_O_TBMR) =
	(HWREG(WTIMER0_BASE+TIMER_O_TBMR)& ~TIMER_TBMR_TBMR_M)|
	TIMER_TBMR_TBMR_PERIOD;
// set timeout to 2mS
	HWREG(WTIMER0_BASE+TIMER_O_TBILR) = TicksPerMS * 2;
// enable a local timeout interrupt
	HWREG(WTIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;
// enable the Timer B in Wide Timer 0 interrupt in the NVIC
// it is interrupt number 95 so appears in EN2 at bit 31
	HWREG(NVIC_EN2) = BIT31HI;
// make sure interrupts are enabled globally
	__enable_irq();
// now kick the timer off by enabling it and enabling the timer to
// stall while stopped by the debugger
	HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN |
	TIMER_CTL_TBSTALL);
}

// Uses Timer 0 A for motor Position PID updates
void InitMotorPositionPID( void ){
// start by enabling the clock to the timer (Timer 0)
	HWREG(SYSCTL_RCGCTIMER) |= SYSCTL_RCGCTIMER_R0;
	while ((HWREG(SYSCTL_RCGCTIMER) & SYSCTL_PRTIMER_R0) != SYSCTL_PRTIMER_R0)
		;
// make sure that timer (Timer A) is disabled before configuring
	HWREG(TIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
// set it up in 16bit wide (individual, not concatenated) mode
	HWREG(TIMER0_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
// set up timer A in periodic mode so that it repeats the time-outs
	HWREG(TIMER0_BASE+TIMER_O_TAMR) =
	(HWREG(TIMER0_BASE+TIMER_O_TAMR)& ~TIMER_TAMR_TAMR_M)|
	TIMER_TAMR_TAMR_PERIOD;
// set timeout to 2mS
	HWREG(TIMER0_BASE+TIMER_O_TAILR) = TicksPerMS * 2;
// enable a local timeout interrupt
	HWREG(TIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_TATOIM;
// enable the Timer A in Timer 0 interrupt in the NVIC
// it is interrupt number 19 so appears in EN0 at bit 19
	HWREG(NVIC_EN0) = BIT19HI;
// make sure interrupts are enabled globally
	__enable_irq();
// now kick the timer off by enabling it and enabling the timer to
// stall while stopped by the debugger
	HWREG(TIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN |
	TIMER_CTL_TASTALL);
}

void InitSPI(void){
	// enable the clock to Port A
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R0;
	// enable the clock to SSI module 0
	HWREG(SYSCTL_RCGCSSI) |= SYSCTL_RCGCSSI_R0;
	// wait for the GPIO port to be ready
	while((HWREG(SYSCTL_RCGCGPIO) & SYSCTL_RCGCGPIO_R0) != SYSCTL_RCGCGPIO_R0)
		;
	// program the GPIO to use the alternate functions on the SSI pins
	HWREG(GPIO_PORTA_BASE+GPIO_O_AFSEL) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI);
	// set mux position in GPIOCTL to select the SSI use of the pins
	HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) =
		(HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) & 0xff0000ff) + (2<<8) + (2<<12) + (2<<16) + (2<<20);
	// program the port lines for digital I/O
	HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI);
	// program the required data directions on the port lines
	HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) &= (BIT4LO); // Receive are inputs
	HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) |= (BIT2HI | BIT5HI | BIT3HI); // Clock, Transfer, and Frame/Slave Select are outputs
	// if using SPI mode 3, program the pull-up on the clock line
	HWREG(GPIO_PORTA_BASE+GPIO_O_PUR) |= (BIT2HI|BIT3HI); // Pullups on clock line and slave select lines
	// wait for the SSI0 to be ready
	while((HWREG(SYSCTL_RCGCSSI) & SYSCTL_RCGCSSI_R0) != SYSCTL_RCGCSSI_R0)
		;
	// make sure that the SSI is disabled before programming mode bits
	HWREG(SSI0_BASE+SSI_O_CR1) &= BIT1LO;
	// select master mode (MS) & TXRIS indicating End of Transmit (EOT)
	HWREG(SSI0_BASE+SSI_O_CR1) &= BIT2LO;
	HWREG(SSI0_BASE+SSI_O_CR1) |= BIT4HI;
	// configure the SSI clock source to the system clock
	HWREG(SSI0_BASE+SSI_O_CC) &= (BIT0LO & BIT1LO & BIT2LO & BIT3LO);
	// configure the clock pre-scaler
	HWREG(SSI0_BASE+SSI_O_CPSR) = (HWREG(SSI0_BASE+SSI_O_CPSR) & 0xffffff00)+220;
	// configure clock rate (SCR), phase & polarity (SPH, SPO), mode (FRF), data size (DSS)
	HWREG(SSI0_BASE+SSI_O_CR0) &= (~SSI_CR0_SCR_M);
	HWREG(SSI0_BASE+SSI_O_CR0) |= (0x00001400); // set SCR to 20
	HWREG(SSI0_BASE+SSI_O_CR0) |= (SSI_CR0_SPH | SSI_CR0_SPO); // set SPH and SPO	to 1
	HWREG(SSI0_BASE+SSI_O_CR0) &= ~SSI_CR0_FRF_M; // set FRF to 0
	HWREG(SSI0_BASE+SSI_O_CR0) |= SSI_CR0_DSS_8;	// set DSS to 0x7
	// locally enable interrupts (TXIM in SSIIM)
	HWREG(SSI0_BASE+SSI_O_IM) |= BIT3HI;
	/***********************************************************************************
	To Do: maybe enable the RXIM interrupt to know when end of receive happens
	***********************************************************************************/
	// make sure that the SSI is enabled for operation
	HWREG(SSI0_BASE+SSI_O_CR1) |= BIT1HI;
	// enable the NVIC interrupt for the SSI when starting to transmit
	HWREG(NVIC_EN0) |= BIT7HI; // SSI0 is interrupt 7, so EN0 bit 1
	// make sure interrupts are enabled globally
	__enable_irq();
}

// Init for city sensor input capture interrupt
void InitInputCaptureCityPeriod( void ){
		// start by enabling the clock to the timer (Wide Timer 2)
	HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R2;
	// enable the clock to Port D
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;
	// since we added this Port D clock init, we can immediately start
	// into configuring the timer, no need for further delay
	// make sure that timer (Timer A) is disabled before configuring
	HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
	// set it up in 32bit wide (individual, not concatenated) mode
	// the constant name derives from the 16/32 bit timer, but this is a 32/64
	// bit timer so we are setting the 32bit mode
	HWREG(WTIMER2_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
	// we want to use the full 32 bit count, so initialize the Interval Load
	// register to 0xffff.ffff (its default value :-)
	HWREG(WTIMER2_BASE+TIMER_O_TAILR) = 0xffffffff;
	// set up timer A in capture mode (TAMR=3, TAAMS = 0),
	// for edge time (TACMR = 1) and up-counting (TACDIR = 1)
	HWREG(WTIMER2_BASE+TIMER_O_TAMR) =
	(HWREG(WTIMER2_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) |
	(TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
	// To set the event to rising edge, we need to modify the TAEVENT bits
	// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
	HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;

	// Now Set up the port to do the capture (clock was enabled earlier)
	// start by setting the alternate function for Port D bit 0 (WT2CCP0)
	HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= BIT0HI;
	// Then, map bit 0's alternate function to WT2CCP0
	// 7 is the mux value to select WT2CCP0, 0 to shift it over to the
	// right nibble for bit 0 (4 bits/nibble * 4 bits)
	HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) =
	(HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xfffffff0) + 7;
	// Enable pin on Port D for digital I/O
	HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= BIT0HI;
	// make pin 0 on Port D into an input
	HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= BIT0LO;
	// back to the timer to enable a local capture interrupt
	HWREG(WTIMER2_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
	// enable the Timer A in Wide Timer 2 interrupt in the NVIC
	// it is interrupt number 98 so appears in EN3 at bit 2
	HWREG(NVIC_EN3) |= BIT2HI;
	// make sure interrupts are enabled globally
	__enable_irq();
	// now kick the timer off by enabling it and enabling the timer to
	// stall while stopped by the debugger
	HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
}


void InitShooterPWM( void ){
	volatile uint32_t Dummy; // use volatile to avoid over-optimization
// start by enabling the clock to the PWM Module (PWM0)
	HWREG(SYSCTL_RCGCPWM) |= SYSCTL_RCGCPWM_R0;
// enable the clock to Port E
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R4;
// Select the PWM clock as System Clock/32
	HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
		(SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_32);
// make sure that the PWM module clock has gotten going
	while ((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
		;
// disable the PWM while initializing
	HWREG( PWM0_BASE+PWM_O_2_CTL ) = 0;
// program generator A to go to 1 at rising compare A, 0 on falling compare A
	HWREG( PWM0_BASE+PWM_O_2_GENA) =
		(PWM_2_GENA_ACTCMPAU_ONE | PWM_2_GENA_ACTCMPAD_ZERO );
// program generator B to go to 1 at rising compare B, 0 on falling compare B
	HWREG( PWM0_BASE+PWM_O_2_GENB) =
		(PWM_2_GENB_ACTCMPAU_ONE | PWM_2_GENB_ACTCMPAD_ZERO );
// Set the PWM period. Since we are counting both up & down, we initialize
// the load register to 1/2 the desired total period. We will also program
// the match compare registers to 1/2 the desired high time
	HWREG( PWM0_BASE+PWM_O_2_LOAD) = ((ServoPeriod * ServoTicksPerMS)-1)>>1;
// enable the PWM outputs
	HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM4EN | PWM_ENABLE_PWM5EN);
// now configure the Port E pins to be PWM outputs
// start by selecting the alternate function for PE4 & 5
	HWREG(GPIO_PORTE_BASE+GPIO_O_AFSEL) |= (BIT4HI | BIT5HI);
// now choose to map PWM to those pins, this is a mux value of 4 that we
// want to use for specifying the function on bits 4 & 5
	HWREG(GPIO_PORTE_BASE+GPIO_O_PCTL) =
		(HWREG(GPIO_PORTE_BASE+GPIO_O_PCTL) & 0xff00ffff) + (4<<(5*BitsPerNibble)) +
		(4<<(4*BitsPerNibble));
// Enable pins 4 & 5 on Port E for digital I/O
	HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |= (BIT4HI | BIT5HI);
// make pins 4 & 5 on Port B into outputs
	HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) |= (BIT4HI |BIT5HI);
// set the up/down count mode, enable the PWM generator and make
// both generator updates locally synchronized to zero count
	HWREG(PWM0_BASE+ PWM_O_2_CTL) = (PWM_2_CTL_MODE | PWM_2_CTL_ENABLE |
		PWM_2_CTL_GENAUPD_LS | PWM_2_CTL_GENBUPD_LS);
}

void InitGameOneShot( void ){ 
// start by enabling the clock to the timer ( Timer 4)
	HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R4;
	while ((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R4) != SYSCTL_PRWTIMER_R4)
		;
// make sure that timer (Timer A) is disabled before configuring
	HWREG(WTIMER4_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
// set it up in 16 bit wide (individual, not concatenated) mode
	HWREG(WTIMER4_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
	HWREG(WTIMER4_BASE+TIMER_O_TAPR) |= 0x2; // prescale by 2
// set up timer A in one-shot mode so that it repeats the time-outs
	HWREG(WTIMER4_BASE+TIMER_O_TAMR) =
	(HWREG(WTIMER4_BASE+TIMER_O_TAMR)& ~TIMER_TAMR_TAMR_M)|
	TIMER_TAMR_TAMR_1_SHOT;
// set timeout to 2mS
  HWREG(WTIMER4_BASE+TIMER_O_TAILR) = 0x6DAC2C00; //0xa4824200;
	//HWREG(WTIMER4_BASE+TIMER_O_TAILR) = 0x1C24200;
// enable a local timeout interrupt
	HWREG(WTIMER4_BASE+TIMER_O_IMR) |= TIMER_IMR_TATOIM;
// enable the Timer A in Timer 1 interrupt in the NVIC
// it is interrupt number 102
	HWREG(NVIC_EN3) |= BIT6HI;
// make sure interrupts are enabled globally
	__enable_irq();
// now kick the timer off by enabling it and enabling the timer to
// stall while stopped by the debugger
	HWREG(WTIMER4_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN |
	TIMER_CTL_TASTALL);
}


#if 0 //Deprecated. Now using ES_Timer for this capability
// Uses Wide Timer 1 B
void InitOneShotLeft( void ){
// start by enabling the clock to the timer ( Timer 1)
	HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R1;
	while ((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R1) != SYSCTL_PRWTIMER_R1)
		;
// make sure that timer (Timer A) is disabled before configuring
	HWREG(WTIMER1_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
// set it up in 16 bit wide (individual, not concatenated) mode
	HWREG(WTIMER1_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
// set up timer A in one-shot mode so that it repeats the time-outs
	HWREG(WTIMER1_BASE+TIMER_O_TBMR) =
	(HWREG(WTIMER1_BASE+TIMER_O_TBMR)& ~TIMER_TBMR_TBMR_M)|
	TIMER_TBMR_TBMR_1_SHOT;
// set timeout to 2mS
	HWREG(WTIMER1_BASE+TIMER_O_TBILR) = 0x7fffff;
// enable a local timeout interrupt
	HWREG(WTIMER1_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;
// enable the Timer A in Timer 1 interrupt in the NVIC
// it is interrupt number 97 so appears in EN at bit 21
	HWREG(NVIC_EN3) = BIT1HI;
// make sure interrupts are enabled globally
	__enable_irq();
// now kick the timer off by enabling it and enabling the timer to
// stall while stopped by the debugger
	HWREG(WTIMER1_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN |
	TIMER_CTL_TBSTALL);
}

// Uses Wide Timer 2 B
void InitOneShotRight( void ){
// start by enabling the clock to the timer ( Timer 2)
	HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R2;
	while ((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R2) != SYSCTL_PRWTIMER_R2)
		;
// make sure that timer (Timer B) is disabled before configuring
	HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
// set it up in 16 bit wide (individual, not concatenated) mode
	HWREG(WTIMER2_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
// set up timer A in one-shot mode so that it repeats the time-outs
	HWREG(WTIMER2_BASE+TIMER_O_TBMR) =
	(HWREG(TIMER2_BASE+TIMER_O_TBMR)& ~TIMER_TBMR_TBMR_M)|
	TIMER_TBMR_TBMR_1_SHOT;
// set timeout to 2mS
	HWREG(WTIMER2_BASE+TIMER_O_TBILR) = 0x7fffff;
// enable a local timeout interrupt
	HWREG(WTIMER2_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;
// enable the Timer A in Timer 1 interrupt in the NVIC
// it is interrupt number 99 so appears in EN3 at bit 3
	HWREG(NVIC_EN3) = BIT3HI;
// make sure interrupts are enabled globally
	__enable_irq();
// now kick the timer off by enabling it and enabling the timer to
// stall while stopped by the debugger
	HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN |
	TIMER_CTL_TBSTALL);
}
#endif