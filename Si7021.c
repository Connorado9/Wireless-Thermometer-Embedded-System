/**
 * @file Si7021.c
 * @author Connor Humiston
 * @date 2/25/20
 * @brief This module is responsible for implementing the Si7021 i2c temperature sensor
 */


//***********************************************************************************
// Include files
//***********************************************************************************
#include "Si7021.h"
#include "i2c.h"
#include "gpio.h"

//***********************************************************************************
// defined files
//***********************************************************************************
#define byte_num 		2

//***********************************************************************************
// global variables
//***********************************************************************************
static uint32_t			raw_data;

//***********************************************************************************
// functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 *	 This function opens Si7021
 * @details
 * 	 Info is passed to I2C open in order to set up the Si7021 as well as interrupts
 ******************************************************************/
void Si7021_i2c_open(void)
{
	I2C_IO_STRUCT local_io;
	local_io.SCL_pin = SI7021_SCL_PIN;
	local_io.SCL_port = SI7021_SCL_PORT;
	local_io.SDA_pin = SI7021_SDA_PIN;
	local_io.SDA_port = SI7021_SCL_PORT;

	I2C_OPEN_STRUCT local_si7021;
	local_si7021.SCL_pin_en = SI7021_SCL_EN ;
	local_si7021.SCL_pin_route = SI7021_SCL_LOC;
	local_si7021.SDA_pin_en = SI7021_SDA_EN;
	local_si7021.SDA_pin_route = SI7021_SDA_LOC;
	local_si7021.clhr = SI7021_I2C_CLK_RATIO;
	local_si7021.enable = true;
	local_si7021.freq = SI7021_I2C_FREQ;
	local_si7021.master = true;
	local_si7021.refFreq = SI7021_REFFREQ;

	i2c_open(SI7021_I2C, &local_si7021, &local_io);
}


/***************************************************************************//**
 * @brief
 *	 This function is responsible for providing the correct info to start the I2C bus for the Si7021
 * @details
 *	 The function creates a local struct and passes it to I2C_Start to initialize and begin the I2C process
 ******************************************************************/
void Si7021_read(void)
{
	I2C_PAYLOAD_INIT local_payload;
	local_payload.peripheral = SI7021_I2C;
	local_payload.bytes = byte_num;
	local_payload.cmd = SI7021_TEMP_NO_HOLD;
	local_payload.data = &raw_data;
	local_payload.device_address = Si7021_dev_addr;

	//void I2C_Start(I2C_TypeDef *i2c_peripheral, I2C_PAYLOAD_STRUCT i2c_load)
	I2C_Start(&local_payload);
}

/***************************************************************************//**
 * @brief
 *	 This function calculates the temperature in degrees C
 * @details
 * 	 This function returns a temperature float value
 ******************************************************************/
float Si7021_temperature_C(void)
{
	float tempC;
	tempC = (raw_data/372.957) - 46.85;
	return tempC;
}


/***************************************************************************//**
 * @brief
 *	 This function calculates the temperature in degrees F from the Celsius measure
 * @details
 * 	 This function returns a temperature float value
 ******************************************************************/
float Si7021_temperature_F(void)
{
	float tempC, tempF;
	tempC = (raw_data/372.957) - 46.85;
	//tempC = (175.2*raw_data)/65536 - 46.85;
	tempF = (1.8*tempC) + 32;
	return tempF;
}
