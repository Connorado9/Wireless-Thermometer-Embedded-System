/**
 * @file i2c.h
 * @date  Feb 20, 2020
 * @author connorhumiston
 * @brief This module defines three structs and function prototypes for setting up and reseting an I2C peripheral
 */



//***********************************************************************************
// Include files
//***********************************************************************************
#ifndef SRC_HEADER_FILES_I2C_H_
#define SRC_HEADER_FILES_I2C_H_
#include "em_i2c.h"
#include "stdbool.h"
#include "em_i2c.h"
#include "em_gpio.h"
#include "sleep_routines.h"

//***********************************************************************************
// defined files
//***********************************************************************************
#define I2C0_SCL_ROUTE_LOC		I2C_ROUTELOC0_SCLLOC_LOC15
#define I2C1_SCL_ROUTE_LOC		I2C_ROUTELOC0_SCLLOC_LOC19

#define I2C0_SDA_ROUTE_LOC		I2C_ROUTELOC0_SDALOC_LOC19
#define I2C1_SDA_ROUTE_LOC		I2C_ROUTELOC0_SDALOC_LOC15

#define I2C_EM_BLOCK			EM2

#define SI7021_READ_EVT			0x00000008	//0b1000
//***********************************************************************************
// global variables
//***********************************************************************************
// Used by a device such as the Si7021 module to open an i2c peripheral
// The STRUCT that the application code will use to define or configure the Pearl Gecko I2C peripheral per the application requirements
typedef struct
{
	I2C_ClockHLR_TypeDef 	clhr;			//Clock low/high ratio control
	bool 					enable;			//Enable I2C peripheral when init completed
	uint32_t 				freq;			//(Max) I2C bus frequency to use. This parameter is only applicable if operating in master mode.
	bool 					master;			//Set to master (true) or slave (false) mode
	uint32_t 				refFreq;			//I2C reference clock assumed when configuring bus frequency setup.
											//Set it to 0 if currently configurated reference clock shall be used. This parameter is only applicable if operating in master mode.
	uint32_t				SDA_pin_route;	// sda route to gpio port/pin
	uint32_t				SCL_pin_route;	// scl route to gpio port/pin
	uint32_t				SDA_pin_en;		// enable SDA pin
	uint32_t				SCL_pin_en;		// enable SCL pin
	//uint8_t				flag;
} I2C_OPEN_STRUCT;

// Used by the program to reset the I2C communication bus
// The STRUCT that contains the pin and port information for the two external I2C signal pins, SCL and SDA.
// This structure will be initialized in the application portion of the project, Si7021.c
//  and used in the i2c.c file to reset the I2C communication bus
typedef struct
{
	uint32_t				SCL_port;
	uint32_t 				SCL_pin;
	uint32_t 				SDA_port;
	uint32_t 				SDA_pin;
} I2C_IO_STRUCT;

// Defines the I2C operation and keeps state of the I2C state machine
typedef enum
{
	write,	//0
	read	//1
} wr_command;

//typedef enum
//{
//	MS,		//0
//	LS		//1
//} byte_location;

typedef enum
{
	initialize,
	send_measure_cmd,
	send_read_cmd,
	receive_data,
	end_process
} i2c_defined_states;

typedef struct
{
	I2C_TypeDef 			*peripheral;	//I2C0 or I2C1
	uint32_t				device_address; //device address?
	i2c_defined_states		current_state;	//current state in machine
	bool					read_write;		//reading (1) or writing (0)
	uint32_t				bytes;			//number of bytes
	uint32_t				*data;			//the actual data
	uint32_t				cmd;			//hold or not
} I2C_PAYLOAD_STRUCT;


typedef struct
{
	I2C_TypeDef 			*peripheral;	//I2C0 or I2C1
	uint32_t				device_address; //device address?
	uint32_t				bytes;			//number of bytes
	uint32_t				*data;			//the actual data
	uint32_t				cmd;			//hold or not
} I2C_PAYLOAD_INIT;

//***********************************************************************************
// function prototypes
//***********************************************************************************
void i2c_open(I2C_TypeDef *i2c_peripheral, I2C_OPEN_STRUCT *i2c_setup, I2C_IO_STRUCT *i2c_io);
void i2c_bus_reset(I2C_TypeDef	*i2c_peripheral, I2C_IO_STRUCT *i2c_io);
void I2C0_IRQHandler(void);
void I2C1_IRQHandler(void);
void I2C_ACK();
void I2C_NACK();
void I2C_RXDATAV();
void I2C_MSTOP();
void I2C_Start(I2C_PAYLOAD_INIT *i2c_load);


#endif /* SRC_HEADER_FILES_I2C_H_ */
