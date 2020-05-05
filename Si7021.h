/**
 * @file Si7021.h
 * @author Connor Humiston
 * @date 2/25/20
 * @brief This module defines necessary components for the Si7021 temperature sensor.
 *
 */

#ifndef SRC_HEADER_FILES_SI7021_H_
#define SRC_HEADER_FILES_SI7021_H_


//***********************************************************************************
// Include files
//***********************************************************************************
#include "cmu.h"
#include "i2c.h"

//***********************************************************************************
// defined files
//***********************************************************************************
#define Si7021_dev_addr			0x40 // Si7021 I2C device address
#define SI7021_TEMP_NO_HOLD  	0xF3 // Si7021 temp read/no hold cmd
#define SI7021_I2C_FREQ 		I2C_FREQ_FAST_MAX
#define SI7021_REFFREQ			0
#define SI7021_I2C_CLK_RATIO 	_I2C_CTRL_CLHR_ASYMMETRIC	//Asymmetric
#define SI7021_SCL_LOC 			I2C_ROUTELOC0_SCLLOC_LOC15//Decides the location of the I2C SCL pin - This should be LOC15 in SCLLOC
#define SI7021_SCL_EN 			I2C_ROUTEPEN_SCLPEN 		//SCL enable according to the ref manual
#define SI7021_SDA_LOC 			I2C_ROUTELOC0_SDALOC_LOC15	//16 //Decides the location of the I2C SDA pin - LOC16 in SDALOC
#define SI7021_SDA_EN 			I2C_ROUTEPEN_SDAPEN 		//SDA enable according to the ref manual
#define SI7021_I2C				I2C0 // The PG I2C peripheral to use

//***********************************************************************************
// global variables
//***********************************************************************************


//***********************************************************************************
// function prototypes
//***********************************************************************************
void Si7021_i2c_open(void);
void Si7021_read(void);
float Si7021_temperature_C(void);
float Si7021_temperature_F(void);

#endif /* SRC_HEADER_FILES_SI7021_H_ */
