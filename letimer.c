/**
 * @file letimer.c
 * @author Connor Humiston
 * @date January 12th, 2020
 * @brief Contains all the LETIMER driver functions
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************

//** Standard Libraries

//** Silicon Lab include files
#include "em_cmu.h"
#include "em_assert.h"

//** User/developer include files
#include "letimer.h"
#include "scheduler.h"

//***********************************************************************************
// defined files
//***********************************************************************************

//***********************************************************************************
// global variables
//***********************************************************************************

//***********************************************************************************
// private variables
//***********************************************************************************
static uint32_t scheduled_comp0_evt;
static uint32_t scheduled_comp1_evt;
static uint32_t scheduled_uf_evt;

//***********************************************************************************
// functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 *   Driver to open and set an LETIMER peripheral in PWM mode
 *
 * @details
 * 	 This routine is a low level driver.  The application code calls this function
 * 	 to open one of the LETIMER peripherals for PWM operation to directly drive
 * 	 GPIO output pins of the device and/or create interrupts that can be used as
 * 	 a system "heart beat" or by a scheduler to determine whether any system
 * 	 functions need to be serviced.
 *
 * @note
 *   This function is normally called once to initialize the peripheral and the
 *   function letimer_start() is called to turn-on or turn-off the LETIMER PWM
 *   operation.
 *
 * @param[in] letimer
 *   Pointer to the base peripheral address of the LETIMER peripheral being opened
 *
 * @param[in] app_letimer_struct
 *   Is the STRUCT that the calling routine will use to set the parameters for PWM
 *   operation
 *
 ******************************************************************************/
// Generic driver that could support any of the LE timers on the Pearl Gecko
void letimer_pwm_open(LETIMER_TypeDef *letimer, APP_LETIMER_PWM_TypeDef *app_letimer_struct)
{
	LETIMER_Init_TypeDef letimer_pwm_values; //this typedefine stores the values used to initialize the peripheral


	/*  Enable the routed clock to the LETIMER0 peripheral */
	if(letimer == LETIMER0)
	{
		CMU_ClockEnable(cmuClock_LETIMER0, true);
	}


	/* Use EFM_ASSERT statements to verify whether the LETIMER clock tree is properly
	 * configured and enabled
	 */
	// Verify whether the LETIMER clock tree properly configured and enabled
	// You must select a register that utilizes the clock enabled to be tested
	letimer->CMD = LETIMER_CMD_START;
	while (letimer->SYNCBUSY); //if the clock is not enabled, the program will remain stuck here (something other than 0 if still writing)
		EFM_ASSERT(letimer->STATUS & LETIMER_STATUS_RUNNING); //the code will be stuck at assert_efm's while(1) loop
	//letimer->CMD = LETIMER_CMD_STOP; // command register
	letimer_start(letimer,false); //We use the function instead of the STOP now
	// you can check that it's running with the STATUS register at bottom of LETIMER section

	while(letimer->SYNCBUSY);
	letimer->CNT = 0;

	// Initialize letimer for PWM operation -- this is where we initialize the values of the typedef struct
	letimer_pwm_values.bufTop = false;				// Comp1 will not be used to load comp0, but used to create an on-time/duty cycle
	letimer_pwm_values.comp0Top = true;				// load comp0 into cnt register when count register underflows enabling continuous looping
	letimer_pwm_values.debugRun = app_letimer_struct->debugRun;
	letimer_pwm_values.enable = app_letimer_struct->enable;
	letimer_pwm_values.out0Pol = 0;					// While PWM is not active out, idle is DEASSERTED, 0
	letimer_pwm_values.out1Pol = 0;					// While PWM is not active out, idle is DEASSERTED, 0
	letimer_pwm_values.repMode = letimerRepeatFree;	// Setup letimer for free running for continuous looping
	letimer_pwm_values.ufoa0 = letimerUFOAPwm;		// Using the HAL documentation, set to PWM mode
	letimer_pwm_values.ufoa1 = letimerUFOAPwm;		// Using the HAL documentation, set to PWM mode

	//Initialize private scheduler variables
	scheduled_comp0_evt = app_letimer_struct->comp0_evt;
	scheduled_comp1_evt = app_letimer_struct->comp1_evt;
	scheduled_uf_evt = app_letimer_struct->uf_evt;


	LETIMER_Init(letimer, &letimer_pwm_values);		// Initialize letimer
	// a while() statement that continues to check the SYNCBUSY register until it returns false, 0,
	// indicating no synchronization is in process.
	while(letimer->SYNCBUSY);


	/* Calculate the value of COMP0 and COMP1 and load these control registers
	 * with the calculated values
	 */
	letimer->COMP0 = app_letimer_struct->period * LETIMER_HZ;
	letimer->COMP1 = app_letimer_struct->active_period * LETIMER_HZ;


	/* Set the REP0 mode bits for PWM operation
	 *
	 * Use the values from app_letimer_struct input argument for ROUTELOC0 and ROUTEPEN enable
	 */
	letimer->REP0 = 1;
	letimer->REP1 = 1;
	//					app_letimer_struct->out_pin_0_en | app_letimer_struct->out_pin1_en; 28; through LETIMER0_ROUTE_OUT0 in app.c
	letimer->ROUTELOC0 = app_letimer_struct->out_pin_route0; // | app_letimer_struct->out_pin_route1;
	letimer->ROUTEPEN = ((app_letimer_struct->out_pin_1_en << 1) * LETIMER_ROUTEPEN_OUT1PEN) |
						 (app_letimer_struct->out_pin_0_en * LETIMER_ROUTEPEN_OUT0PEN);
	//					0b01; true;


	/************************** INTERRUPTS **************************/
	//Clearing all interrupts that could have been there
	letimer->IFC |= LETIMER_IFC_COMP0;
	letimer->IFC |= LETIMER_IFC_COMP1;
	letimer->IFC |= LETIMER_IFC_UF;

	//Enabling the interrupt function
	if(app_letimer_struct->comp0_irq_enable)
	{
		letimer->IEN |= LETIMER_IEN_COMP0;
	}
	if(app_letimer_struct->comp1_irq_enable)
	{
		letimer->IEN |= LETIMER_IEN_COMP1;
	}
	if(app_letimer_struct->uf_irq_enable)
	{
		letimer->IEN |= LETIMER_IEN_UF;
	}

	//Enabling NVIC LETIMER0 Interrupt in ARM Cortex CPU to allow requests
	NVIC_EnableIRQ(LETIMER0_IRQn);

	//If the current status bit is the same as the runnning, then we block
	if(letimer->STATUS & LETIMER_STATUS_RUNNING)
	{
		sleep_block_mode(LETIMER_EM);
	}

	/* We will not enable the LETIMER0 at this time */

}


/***************************************************************************//**
 * @brief
 *   Used to enable, turn-on, or disable, turn-off, the LETIMER peripheral
 *
 * @details
 * 	 This function allows the application code to initialize the LETIMER
 * 	 peripheral separately to enabling or disabling the LETIMER
 *
 * @note
 *   Application code should not directly access hardware resources.  The
 *   application program should access the peripherals through the driver
 *   functions
 *
 * @param[in] letimer
 *   Pointer to the base peripheral address of the LETIMER peripheral being
 *   enabled or disable
 *
 * @param[in] enable
 *   true enables the LETIMER to start operation while false disables the
 *   LETIMER
 *
 ******************************************************************************/
void letimer_start(LETIMER_TypeDef *letimer, bool enable)
{
	if(enable && (letimer->STATUS & ~LETIMER_STATUS_RUNNING))
	{
		//if not running & enabling timer, the energy mode should be blocked
		sleep_block_mode(LETIMER_EM);
	}
	if(!enable && (letimer->STATUS & LETIMER_STATUS_RUNNING))
	{
		//if running & disabling timer, the energy mode should be unblocked
		sleep_unblock_mode(LETIMER_EM);
	}
	LETIMER_Enable(letimer, enable);
	while(letimer->SYNCBUSY); //guarantees that the LETIMER_Enable() operation completes by adding a stall
		//stalls until the LETIMER has synchronized the CMD register write.
}


/***************************************************************************//**
 * @brief
 *	This function has the interrupt service routine for LETIMER0
 *
 * @details
 *	In this function, events are scheduled through the Interrupt Service Routine
 *	and Events are processed or serviced through event handlers called by the scheduler function
 *
 * @note
 *   The Dev Tools (development tools) aka the Linker will replace any default function
 *   with a user defined function if the names are identical.
 *   Therefore this must have its exact name
 *
 ******************************************************************************/
void LETIMER0_IRQHandler(void)
{
	//Declaring a local variable to store the source interrupts
	uint32_t int_flag;

	//Determining which interrupt was raised
	int_flag = LETIMER0->IF & LETIMER0->IEN; //AND interrupt source (IF register) and enable for interrupts we are interested in only

	//Clearing the interrupts with Interrupt Flag Clear Register so they can occur again
	LETIMER0->IFC = int_flag;

	if(int_flag & LETIMER_IF_COMP0)
	{
		//LETIMER0->IFC = LETIMER_IFC_COMP0;
		EFM_ASSERT(!(LETIMER0->IF & LETIMER_IF_COMP0));
		//Adding event to the scheduler, upon exiting the interrupt handler, you program will go back to the scheduler and process all desired events.
		add_scheduled_event(scheduled_comp0_evt);
						  //LETIMER0_COMP0_EVT
	}
	if(int_flag & LETIMER_IF_COMP1)
	{
		//LETIMER0->IFC = LETIMER_IFC_COMP1;
		EFM_ASSERT(!(LETIMER0->IF & LETIMER_IF_COMP1));
		//Adding event to the scheduler, upon exiting the interrupt handler, you program will go back to the scheduler and process all desired events.
		add_scheduled_event(scheduled_comp1_evt);
		 	 	 	 	   //LETIMER0_COMP1_EVT
	}
	if(int_flag & LETIMER_IF_UF)
	{
		//LETIMER0->IFC = LETIMER_IFC_UF;
		EFM_ASSERT(!(LETIMER0->IF & LETIMER_IF_UF));
		//Adding event to the scheduler, thus upon exiting the interrupt handler, you program will go back to the scheduler and process all desired events.
		add_scheduled_event(scheduled_uf_evt);
						  //LETIMER0_UF_EVT
	}
}

