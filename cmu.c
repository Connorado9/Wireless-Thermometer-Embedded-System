/**
 * @file cmu.c
 * @author Connor Humiston
 * @date 1/30/20
 * @brief This module is responsible for enabling all oscillators and routing the clock tree for the application.
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "cmu.h"

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// global variables
//***********************************************************************************


//***********************************************************************************
// functions
//***********************************************************************************


/**************************************************************//**
 *
 * @brief
 *		This function is responsible for managing clocks.
 *		It enables the correct clock and oscillator,
 *		and selects the correct clock further down the clock tree
 *
 * @details
 * 		It is important to select the correct oscillator and clock for your application.
 *
 * @note
 *		This function returns nothing and takes no inputs.
 *
 ******************************************************************/
void cmu_open(void)
{
		CMU_ClockEnable(cmuClock_HFPER, true); 					// Enabling the High Frequency Peripheral Clock Tree for I2C (also used for GPIO bus)
		CMU_ClockEnable(cmuClock_CORELE, true);					// Enable the Low Freq clock tree

		CMU_OscillatorEnable(cmuOsc_LFXO, true, true);			// Enable LFXO for LEUART
		CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);		// Selecting the correct branch from the LFXO tree

		// By default, LFRCO is enabled, disable the LFRCO oscillator
		CMU_OscillatorEnable(cmuOsc_LFRCO, false, false);		// using LFXO or ULFRCO
		CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);		// route ULFRCO to proper Low Freq clock tree
}

