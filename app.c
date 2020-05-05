/**
 * @file app.c
 * @author Connor Humiston
 * @date 1/30/20
 * @brief
 * 		app.c sets up peripherals and defines an important struct for the letimer periods
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "app.h"
#include "letimer.h"
#include "scheduler.h"
#include "sleep_routines.h"
#include "Si7021.h"
#include "i2c.h"
#include "leuart.h"
#include "ble.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <string.h>
#include <stdio.h>


//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// global variables
//***********************************************************************************


//***********************************************************************************
// function
//***********************************************************************************


/***************************************************************************//**
 * @brief
 *		Sets up the peripherals.
 * @details
 *		app_peripheral_setup sets up peripherals by using functions to enable the correct clocks and oscillators,
 *		set GPIO drive strengths, and pass pwm periods to a function that populates a struct with them
 * @note
 *		This function requires PWM Period and Active Periods to be defined prior
 ******************************************************************************/
void app_peripheral_setup(void)
{
	cmu_open();
	gpio_open();
	scheduler_open();
	app_letimer_pwm_open(PWM_PER, PWM_ACT_PER);
	Si7021_i2c_open();
	ble_circ_init();
	add_scheduled_event(BOOT_UP_EVT);
	ble_open(LEUART_TX_EVT, LEUART_RX_EVT);
}


/***************************************************************************//**
 * @brief
 *		This function populates a struct with details to configure LETIMER0
 * @details
 *		The app_letimer_structure connects predefined data with a structure that is passed to LETIMER0's driver.
 * @note
 *		LETIMER0 is being initialized for PWM operation
 * @param[in] period
 *		The PWM period in seconds
 * @param[in]
 * 		The PWM active period in seconds
 ******************************************************************************/
void app_letimer_pwm_open(float period, float act_period)
{
	//Declaration of the struct:
	// This STRUCT contains additional information to complete the configuration of the LETIMER0
	// peripheral than what is included in the LETIMER_Init_TypeDef
	// For example, the period and active_period elements in APP_LETIMER_PWM_TypeDef are not required for the LETIMER_Init_TypeDef,
	// but are required to initialize the values of the LETIMER0 COMP0 and COMP1 registers.
	APP_LETIMER_PWM_TypeDef app_letimer_structure;

	app_letimer_structure.period = PWM_PER;
	app_letimer_structure.active_period = PWM_ACT_PER;
	app_letimer_structure.debugRun = false; //
	app_letimer_structure.enable = false; //
	app_letimer_structure.out_pin_0_en = LETIMER0_OUT0_EN;
	app_letimer_structure.out_pin_1_en = LETIMER0_OUT1_EN;
	app_letimer_structure.out_pin_route0 = LETIMER0_ROUTE_OUT0;
	app_letimer_structure.out_pin_route1 = LETIMER0_ROUTE_OUT1;

	app_letimer_structure.comp0_irq_enable = false;
	app_letimer_structure.comp0_evt = LETIMER0_COMP0_EVT;
	app_letimer_structure.comp1_irq_enable = false;
	app_letimer_structure.comp1_evt = LETIMER0_COMP1_EVT;
	app_letimer_structure.uf_irq_enable = true;
	app_letimer_structure.uf_evt = LETIMER0_UF_EVT;

	letimer_pwm_open(LETIMER0, &app_letimer_structure);
}

/***************************************************************************//**
 * @brief
 *		Event handler for LETIMER0 UF
 * @details
 *		This function acts as the event handler for the LETIMER0 event for UF
 * @note
 * 		In the event handler, we clear/remove the event due to that the event is now being processed or serviced.
 * 		By clearing the event, we are making it available to be called the next time the event is triggered.
 ******************************************************************************/
void scheduled_letimer0_uf_evt (void)
{
//	uint32_t currEM = current_block_energy_mode();
//	sleep_unblock_mode(currEM);
//	if(currEM < 4) //if current energy mode block less than 4, set to EM + 1
//		sleep_block_mode(currEM + 1);
//	else //if current energy mode block is 4, set to EM0
//		sleep_block_mode(EM0);

	EFM_ASSERT(get_scheduled_events() & LETIMER0_UF_EVT); //This assert will verify whether the event handler was entered due to the proper event.
	remove_scheduled_event(LETIMER0_UF_EVT);
	Si7021_read();

	//if NOT this then you could check that the scheduled event was not removed
}

/***************************************************************************//**
 * @brief
 *		Event handler for LETIMER0 COMP0 interrupt
 * @note
 * 		In the event handler, we clear/remove the event due to that the event is now being processed or serviced.
 * 		By clearing the event, we are making it available to be called the next time the event is triggered.
 ******************************************************************************/
void scheduled_letimer0_comp0_evt (void)
{
	remove_scheduled_event(LETIMER0_COMP0_EVT);
	EFM_ASSERT(false);
	//The false statement will always return a 0 and thus the Assert will fail which should happen because comp0 is not interrupting now
}

/***************************************************************************//**
 * @brief
 *		Event handler for LETIMER0 COMP1 interrupt
 * @note
 * 		In the event handler, we clear/remove the event due to that the event is now being processed or serviced.
 * 		By clearing the event, we are making it available to be called the next time the event is triggered.
 ******************************************************************************/
void scheduled_letimer0_comp1_evt (void)
{
	remove_scheduled_event(LETIMER0_COMP1_EVT);
	EFM_ASSERT(false);
	//The false statement will always return a 0 and thus the Assert will fail which should happen because comp1 is not interrupting now
}

/***************************************************************************//**
 * @brief
 *		Event handler for the Si7021 temperature sensor
 * @details
 * 	 	If the temperature is greater than 80 degrees F,  the LED1 turns on
 * @note
 * 		In the event handler, we clear/remove the event due to that the event is now being processed or serviced.
 * 		By clearing the event, we are making it available to be called the next time the event is triggered.
 ******************************************************************************/
void Si7021_temp_done_evt(void)
{
	EFM_ASSERT(get_scheduled_events() & SI7021_READ_EVT);
	remove_scheduled_event(SI7021_READ_EVT);

	//Get the temperature values
	float tmp_result_F = Si7021_temperature_F();
	float tmp_result_C = Si7021_temperature_C();

	//Determine if the LED should be lit or not
	if(tmp_result_F >= 80.0)
	{
		//Assert GPIO pin to LED1
		GPIO_PinOutSet(LED1_port, LED1_pin);
	}
	else
	{
		//De-assert GPIO pin to LED1
		GPIO_PinOutClear(LED1_port, LED1_pin);
	}

	//Decide F or C and then prepare the output for ble_write
	char rx_string[100];
	memset(rx_string, 0, 100);
	rx_str_copy(rx_string); //copying the leuart's string received into rx_string to be processed
	//if(rx_string[0] != 0)	ble_write(rx_string);

	bool setting;
	if(strcmp(rx_string, "#F?") == 0)
	{
		setting = false;
	}
	else if(strcmp(rx_string, "#C?") == 0)
	{
		setting = true;
	}

	char str_out[100];
	float current_temp;
	if(setting == false)
	{
		current_temp = (double)tmp_result_F;
		sprintf(str_out, "\nTemp = %4.1f F", current_temp);
	}
	else// if(setting == true)
	{
		current_temp = (double)tmp_result_C;
		sprintf(str_out, "\nTemp = %4.1f C", current_temp);
	}

	//Send it out
	ble_write(str_out);
}

/***************************************************************************//**
 * @brief
 *		Event handler for the boot  up
 * @note
 * 		This event is set from the program not an ISR
 ******************************************************************************/
void scheduled_boot_up_evt(void)
{
	EFM_ASSERT(get_scheduled_events() & BOOT_UP_EVT);
	remove_scheduled_event(BOOT_UP_EVT);
	//letimer_start(LETIMER0, true);

	#ifdef BLE_TEST_ENABLED
	bool test = ble_test("Connors_Test");
	EFM_ASSERT(test);

	for (int i = 0; i < 20000000; i++); //delay to test optimization
	#endif

	leuart_rx_test(LEUART0);

	circular_buff_test();

	letimer_start(LETIMER0, true);
	ble_write("\nHello World!");
	ble_write("\nDDL Course Project");
	ble_write("\nby Connor Humiston");
}


/***************************************************************************//**
 * @brief
 *		Event handler for the the leaurt0 transmission
 * @details
 *		This function calls letimer_start to initialize LETIMER0 and removes the TX event from the scheduler
 ******************************************************************************/
void scheduled_leuart0_tx_done_evt(void)
{
	letimer_start(LETIMER0, true);
	remove_scheduled_event(LEUART_TX_EVT);

	//Upon completion of the LEUART transmission, the LEUART_DONE_EVT event should be set
	//by the LEUART state machine through the add scheduler function.
	//In the LEUART_DONE_EVT event handler, it will need to call the ble_circ_pop() function
	//to check whether another string must be popped off and sent to the LEUART

	ble_circ_pop(false);
	//if(ble_circ_pop())

}
