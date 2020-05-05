/**
 * @file gpio.h
 * @date 1/30/20
 * @brief
 * 		Defines LED 0 & 1 ports
 */

#ifndef SRC_HEADER_FILES_GPIO_H
#define SRC_HEADER_FILES_GPIO_H

//***********************************************************************************
// Include files
//***********************************************************************************
#include "em_gpio.h"

//***********************************************************************************
// defined files
//***********************************************************************************

// LED0 pin is
#define	LED0_port				gpioPortF
#define LED0_pin				04u
#define LED0_default			false 	// off
// LED1 pin is
#define LED1_port				gpioPortF
#define LED1_pin				05u
#define LED1_default			false	// off

// I2C
//SCL
#define SI7021_SCL_PORT 		gpioPortC
#define SI7021_SCL_PIN 			11
#define SI7021_SCL_default		true	//is connecting
//SDA
#define SI7021_SDA_PORT 		gpioPortC
#define SI7021_SDA_PIN 			10
#define SI7021_SDA_default		true	//is connecting
//Sensor Enables - used to select via an external device to the Pearl Whether the Pear pins are connected to the Si7021
#define SI7021_SENSOR_EN_PORT 	gpioPortB
#define SI7021_SENSOR_EN_PIN 	10

// UART
#define UART_RX_PORT			gpioPortD	//LEU0_RX #18 receive input
#define UART_RX_PIN				11
#define UART_TX_PORT			gpioPortD	//LEU0_TX #18 transmit output
#define UART_TX_PIN				10


//***********************************************************************************
// global variables
//***********************************************************************************


//***********************************************************************************
// function prototypes
//***********************************************************************************
void gpio_open(void);

#endif
