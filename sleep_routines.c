/**
 * @file sleep_routines.c
 * @date 2/11/20
 * @author Simplicity Studios & Connor Humiston
 * @brief sleep_routines.c defines functions for going to sleep and selecting energy modes before and after
 * @section License
 *	<b>(C) Copyright 2015 Silicon Labs, http://www.silabs.com</b>
 *	Permission is granted to anyone to use this software for any purpose,
 *	including commercial applications, and to alter it and redistribute it
 *	freely, subject to the following restrictions: *
 *	1. The origin of this software must not be misrepresented; you must not
 *	claim that you wrote the original software.
 *	2. Altered source versions must be plainly marked as such, and must not be
 *	misrepresented as being the original software.
 *	3. This notice may not be removed or altered from any source distribution.
 *	DISCLAIMER OF WARRANTY LIMITATION OF REMEDIES: Silicon Labs has no
 *	obligation to support this Software. Silicon Labs is providing the
 *	Software "AS IS", with no express or implied warranties of any kind,
 *	including, but not limited to, any implied warranties of merchantability
 *	or fitness for any particular purpose or warranties against infringement
 *	of any proprietary rights of a third party.
 *	Silicon Labs will not be liable for any consequential, incidental, or
 *	special damages, or any other relief, or for any claim by any third party,
 *	arising from your use of this Software.
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "sleep_routines.h"

//***********************************************************************************
// Private Variables
//***********************************************************************************
static int lowest_energy_mode[MAX_ENERGY_MODES];

//***********************************************************************************
// Functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 *		Initializes sleep_routines
 * @details
 *		Initializes the sleep_routines private variable, lowest_energy_mode[];
 ******************************************************************************/
void sleep_open(void)
{
	for(int i = 0; i < MAX_ENERGY_MODES; i++)
	{
		lowest_energy_mode[i] = 0;
	}
}

/***************************************************************************//**
 * @brief
 *		Blocks the Pearl Gecko from sleeping
 * @details
 *		Utilized by a peripheral to prevent the Pearl Gecko going into that sleep mode while the peripheral is active.
 *		lowest_energy_mode[EM] is blocked from sleeping by adding to it so it becomes nonzero.
 *		A zero means the EM
 * @param[in] EM
 * 		The energy mode for which the Gecko should not sleep
 ******************************************************************************/
void sleep_block_mode(uint32_t EM)
{
	lowest_energy_mode[EM]++;

	EFM_ASSERT(lowest_energy_mode[EM] < 10); //EFM Assert ensuring that lowest_energy_mode[] doesn't become too large
}

/***************************************************************************//**
 * @brief
 *		Unblocks the Pearl Gecko from sleeping
 * @details
 *		Utilized to release the processor from going into a sleep mode with a peripheral is no longer active.
 * @param[in] EM
 * 		The energy mode
 ******************************************************************************/
void sleep_unblock_mode(uint32_t EM)
{
	if(lowest_energy_mode[EM] > 0)
	{
		lowest_energy_mode[EM]--;
	}

	EFM_ASSERT(lowest_energy_mode[EM] >= 0); //EFM Assert ensuring that lowest_energy_mode[] never goes below zero
}

/***************************************************************************//**
 * @brief
 *		Function to enter sleep
 * @details
 *		if EM0 is blocked, we return
 *		if EM1 is blocked, we also return
 *		if EM2 is blocked, we enter EM1
 *		if EM3 is blocked, we enter EM2 and save the current state
 *		otherwise, we enter EM3, the lowest functional energy mode
 ******************************************************************************/
void enter_sleep(void)
{
	if(lowest_energy_mode[EM0] > 0)
	{
		return;
	}
	else if(lowest_energy_mode[EM1] > 0)
	{
		return;
	}
	else if(lowest_energy_mode[EM2] > 0)
	{
		EMU_EnterEM1();
		return;
	}
	else if(lowest_energy_mode[EM3] > 0)
	{
		EMU_EnterEM2(true);
		return;
	}
	else
	{
		EMU_EnterEM3(true);
		return;
	}
}

/***************************************************************************//**
 * @brief
 *		Function that returns which energy mode that the current system cannot enter.
 * @return
 * 		The uint32_t energy mode the system cannot enter
 ******************************************************************************/
uint32_t current_block_energy_mode(void)
{
	int i = 0;
	while(i < MAX_ENERGY_MODES)
	{
		if(lowest_energy_mode[i] != 0)
		{
			return i;
		}
		i++;
	}
	return (MAX_ENERGY_MODES - 1);
}


