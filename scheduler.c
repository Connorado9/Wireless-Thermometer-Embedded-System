/**
 * @file scheduler.c
 * @date 2/11/20
 * @author: connorhumiston
 * @brief
 * 		scheduler.h defines functions for scheduling and avoids interrupt incoherencies
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "scheduler.h"
#include "em_emu.h"
#include "em_assert.h"

//***********************************************************************************
// Private Variables
//***********************************************************************************
static unsigned int event_scheduled;


//***********************************************************************************
// Functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 *		Opens the scheduler functionality
 * @details
 *		Resets the private variable event_scheduled to 0
 ******************************************************************************/
void scheduler_open(void)
{
//	__disable_irq();
	event_scheduled = 0;
//	__enable_irq();
}

/***************************************************************************//**
 * @brief
 *		Adds a new event to the scheduler
 * @details
 *		ORs a new event, the input argument, into the existing state of the private variable event_scheduled.
 * @param[in] event
 * 		the event parameter includes the bits of the event being added
 ******************************************************************************/
void add_scheduled_event(uint32_t event)
{
	__disable_irq();
	event_scheduled |= event; //adds an event by ORing the event bits to the scheduler
	__enable_irq();
}

/***************************************************************************//**
 * @brief
 *		Removes an event from the scheduler
 * @details
 *		Removes the event, the input argument, from the existing state of the private variable
 * @param[in] event
 * 		the event parameter includes the bits of the event being removed
 ******************************************************************************/
void remove_scheduled_event(uint32_t event)
{
	__disable_irq();
	event_scheduled &= ~event; //negates/removes the event bits using an AND NOT
	__enable_irq();
}

/***************************************************************************//**
 * @brief
 *		Returns the currently scheduled events
 * @details
 *		Returns the current state of the private variable event_scheduled.
 ******************************************************************************/
uint32_t get_scheduled_events(void)
{
	return event_scheduled;
}



