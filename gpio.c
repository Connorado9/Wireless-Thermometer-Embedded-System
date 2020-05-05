/**
 * @file gpio.c
 * @author Connor Humiston
 * @date 1/30/20
 * @brief This module is responsible for enabling the gpio clock and setting GPIO drive strenghts & pins
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "gpio.h"
#include "em_cmu.h"

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// global variables
//***********************************************************************************


//***********************************************************************************
// function prototypes
//***********************************************************************************


//***********************************************************************************
// functions
//***********************************************************************************

/***************************************************************************//**
 *
 * @brief
 *		This function is responsible for enabling the gpio clock and setting GPIO drive strenghts & pins
 *
 * @note
 *		This function returns nothing and takes no inputs.
 *
 ******************************************************************/
void gpio_open(void){

	CMU_ClockEnable(cmuClock_GPIO, true);

	// Set LED ports to be standard output drive with default off (cleared)
	//LED0
	GPIO_DriveStrengthSet(LED0_port, gpioDriveStrengthStrongAlternateStrong);
	//	GPIO_DriveStrengthSet(LED0_port, gpioDriveStrengthWeakAlternateWeak);
	GPIO_PinModeSet(LED0_port, LED0_pin, gpioModePushPull, LED0_default);
	//LED1
	GPIO_DriveStrengthSet(LED1_port, gpioDriveStrengthStrongAlternateStrong);
	//	GPIO_DriveStrengthSet(LED1_port, gpioDriveStrengthWeakAlternateWeak);
	GPIO_PinModeSet(LED1_port, LED1_pin, gpioModePushPull, LED1_default);


	//Set Si7021 Temp Sensor drive strength and pin modes
	//SENSOR_ENABLE
	GPIO_DriveStrengthSet(SI7021_SENSOR_EN_PORT, gpioDriveStrengthWeakAlternateWeak);
	GPIO_PinModeSet(SI7021_SENSOR_EN_PORT, SI7021_SENSOR_EN_PIN, gpioModePushPull, true); //enable is a push pull
	//SDA SCL
	GPIO_PinModeSet(SI7021_SCL_PORT, SI7021_SCL_PIN, gpioModeWiredAnd, SI7021_SCL_default); //mode is wired And
	GPIO_PinModeSet(SI7021_SDA_PORT, SI7021_SDA_PIN, gpioModeWiredAnd, SI7021_SDA_default);


	//LEUART
	//TX
	GPIO_DriveStrengthSet(UART_TX_PORT, gpioDriveStrengthStrongAlternateWeak);
	GPIO_PinModeSet(UART_TX_PORT, UART_TX_PIN, gpioModePushPull, true);
	//RX
	GPIO_PinModeSet(UART_RX_PORT, UART_RX_PIN, gpioModeInput, true);
	//No drive strength for input


}
