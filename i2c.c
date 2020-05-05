/**
 * @file i2c.c
 * @author Connor Humiston
 * @date 2/20/20
 * @brief This module is responsible for driving the i2c
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "i2c.h"
#include "em_assert.h"
#include "em_cmu.h"
//#include "app.h"
#include "sleep_routines.h"
#include "scheduler.h"

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// (private) global variables
//***********************************************************************************
static I2C_PAYLOAD_STRUCT payload;


//***********************************************************************************
// prototypes
//***********************************************************************************


/***************************************************************************//**
 * @brief
 *	i2c_open initializes the sets up the I2C peripheral it is passed.
 * @details
 *	 This function takes in 3 structs to enable the correct I2C clock, verify proper clock operation,
 *	 initialize the I2C peripheral, and set up interrupts for I2C.
 * @note
 *	 Works for I2C0 and I2C1
 * @param[in] i2c_peripheral
 * 	 Similar to the LETIMER_TypeDef, this STRUCT pointer will be used to send the base address of the
 * 	 desired I2Cx peripheral, such as I2C0, I2C1, etc.
 * @param[in] i2c_setup
 *   This input passes in the I2C definition variables and SDA/SCL routing information.
 * @param[in] i2c_io
 *   Passes the external Pearl Gecko port and pin information for SCL and SDA.
 ******************************************************************/
void i2c_open(I2C_TypeDef *i2c_peripheral, I2C_OPEN_STRUCT *i2c_setup, I2C_IO_STRUCT *i2c_io)
{
	//Determining which I2C peripheral is attempting to be opened, and then enabling its clock
	if(i2c_peripheral == I2C0)
	{
		CMU_ClockEnable(cmuClock_I2C0, true); //CMU_HFPEREN0.I2C0
	}
	else if(i2c_peripheral == I2C1)
	{
		CMU_ClockEnable(cmuClock_I2C1, true);
	}

	//Verifying proper clock operation
	if ((i2c_peripheral->IF & 0x01) == 0)
	{
		i2c_peripheral->IFS = 0x01;
		EFM_ASSERT(i2c_peripheral->IF & 0x01);
		i2c_peripheral->IFC = 0x01;
	}
	else
	{
		i2c_peripheral->IFC = 0x01;
		EFM_ASSERT(!(i2c_peripheral->IF & 0x01));
	}

	//Initialize the I2C peripheral
	I2C_Init_TypeDef i2c_temp;
	i2c_temp.clhr = i2c_setup->clhr;
	i2c_temp.enable = i2c_setup->enable;
	i2c_temp.freq = i2c_setup->freq;
	i2c_temp.master = i2c_setup->master;
	i2c_temp.refFreq = i2c_setup->refFreq;
	I2C_Init(i2c_peripheral, &i2c_temp);


	//Route the SCL and SDA signals from internal I2C peripheral to appropriate pearl gecko pins
	i2c_peripheral->ROUTELOC0 = i2c_setup->SCL_pin_route | i2c_setup->SDA_pin_route;
	//i2c_peripheral->ROUTELOC0 = (i2c_setup->SCL_pin_route<<8) | i2c_setup->SDA_pin_route;
	i2c_peripheral->ROUTEPEN = (i2c_setup->SCL_pin_en) | (i2c_setup->SDA_pin_en);
	//i2c_peripheral->ROUTEPEN = (i2c_setup->SCL_pin_en<<1 * I2C_ROUTEPEN_SCLPEN) | (i2c_setup->SDA_pin_en * I2C_ROUTEPEN_SDAPEN);


	i2c_bus_reset(i2c_peripheral, i2c_io);

	//Clear interrupt (with function instead of Interrupt Flag Clear register)
	I2C_IntClear(i2c_peripheral, (I2C_IFC_NACK| I2C_IFC_ACK | I2C_IFC_MSTOP));
	i2c_peripheral->RXDATA; //clears by reading

	//Enable the interrupts
	I2C_IntEnable(i2c_peripheral, (I2C_IEN_NACK| I2C_IEN_ACK | I2C_IEN_MSTOP | I2C_IEN_RXDATAV));

	//Enabling the interrupts at CPU level
	if(i2c_peripheral == I2C0)
	{
		NVIC_EnableIRQ(I2C0_IRQn);
	}
	else
	{
		NVIC_EnableIRQ(I2C1_IRQn);
	}

	//i2c_bus_reset(i2c_peripheral, i2c_io);
}

/***************************************************************************//**
 * @brief
 *	 i2c_bus_reset resets the I2C buses.
 * @details
 *	 Resets the I2C state machines of the Pearl Gecko I2C peripheral
 *	 as well as the I2C state machines of the external I2C devices such as the Si7021.
 * @param[in] i2c_peripheral
 * 	 Specifies which I2C internal peripheral.
 * @param[in] i2c_io
 *   Passes external Pearl Gecko port and pin information for SCL and SDA.
 ******************************************************************/
void i2c_bus_reset(I2C_TypeDef *i2c_peripheral, I2C_IO_STRUCT *i2c_io)
{
	//Before resetting the I2C bus, we must verify that that the I2C SCL and SDA lines are both HIGH, in their inactive state
	EFM_ASSERT(GPIO_PinInGet(i2c_io->SCL_port , i2c_io->SCL_pin));
	EFM_ASSERT(GPIO_PinInGet(i2c_io->SDA_port, i2c_io->SDA_pin));
	//If both asserts pass, then we know the I2C lines are both idle

	uint8_t i;

	//I2C state machine resets after toggling SCL 9 times
	GPIO_PinOutSet(i2c_io->SDA_port, i2c_io->SDA_pin);
	for(i = 0; i < 9; i++)
	{
		GPIO_PinOutClear(i2c_io->SCL_port, i2c_io->SCL_pin);
		GPIO_PinOutSet(i2c_io->SCL_port, i2c_io->SCL_pin);
	}
	//GPIO_PinOutSet(i2c_io->SCL_port, i2c_io->SCL_pin);
	i2c_peripheral->CMD = I2C_CMD_ABORT;
}


/***************************************************************************//**
 * @brief
 *	 Interrupt Service Routine, ISR, Handler for I2C0 peripheral
 * @details
 *	 This function temporarily disables interrupts, determines which interrupt occurred and stores it before clearing the interrupt,
 *	 and then calls the appropriate interrupt handler function
 * @note
 * 	 This function handles interrupts for I2C0
 ******************************************************************/
void I2C0_IRQHandler(void)
{
	//temporarily disable interrupts
	__disable_irq();

	//Locally store the source interrupts & determining which interrupt was raised
	uint32_t int_flag = I2C0->IF & I2C0->IEN; //AND interrupt source (IF register) and enable for interrupts we are interested in only

	//Clearing the current interrupts with Interrupt Flag Clear Register so they can occur again
	I2C0->IFC = int_flag;

	if(int_flag & I2C_IF_ACK)
	{
		I2C_ACK();
	}
	if(int_flag & I2C_IF_NACK)
	{
		I2C_NACK();
	}
	if(int_flag & I2C_IF_RXDATAV)
	{
		I2C_RXDATAV();
	}
	if(int_flag & I2C_IF_MSTOP)
	{
		I2C_MSTOP();
	}

	//Re-enable interrupts
	__enable_irq();
}

/***************************************************************************//**
 * @brief
 *	 Interrupt Service Routine, ISR, Handler for I21 peripheral
 * @details
 *	 This function temporarily disables interrupts, determines which interrupt occurred and stores it before clearing the interrupt,
 *	 and then calls the appropriate interrupt handler function
 * @note
 * 	 This function handles interrupts for I2C1
 ******************************************************************/
void I2C1_IRQHandler(void)
{
	//temporarily disable interrupts
	__disable_irq();

	//Locally store the source interrupts & determining which interrupt was raised
	uint32_t int_flag = I2C1->IF & I2C1->IEN; //AND interrupt source (IF register) and enable for interrupts we are interested in only

	//Clearing the current interrupts with Interrupt Flag Clear Register so they can occur again
	I2C1->IFC = int_flag;

	if(int_flag & I2C_IF_ACK)
	{
		I2C_ACK();
	}
	if(int_flag & I2C_IF_NACK)
	{
		I2C_NACK();
	}
	if(int_flag & I2C_IF_RXDATAV)
	{
		I2C_RXDATAV();
	}
	if(int_flag & I2C_IF_MSTOP)
	{
		I2C_MSTOP();
	}

	//Re-enable interrupts
	__enable_irq();
}


/***************************************************************************//**
 * @brief
 *	 Initializes a private variable in i2c that will keep state of the progress of the I2C operation.
 * @details
 *	 This state information will be stored in a static STRUCT in i2c.c of type I2C_PAYLOAD. (the read of the Si7021 read temperature register)
 * @param[in] i2c_load
 * 	 The information required to initialize I2C_PAYLOAD_STRUCT
 ******************************************************************/
void I2C_Start(I2C_PAYLOAD_INIT *i2c_load)
{
	EFM_ASSERT((i2c_load->peripheral->STATE & _I2C_STATE_STATE_MASK) == I2C_STATE_STATE_IDLE);
//	EFM_ASSERT((I2Cx->STATE & _I2C_STATE_STATE_MASK) == I2C_STATE_STATE_IDLE); // X = the I2C peripheral #
	sleep_block_mode(I2C_EM_BLOCK);				//block up to the I2C M2 mode

	//Initializing
	payload.peripheral = i2c_load->peripheral;									//given i2c peripheral
	payload.device_address = i2c_load->device_address;							//passed address
//	payload.read_write = i2c_load->read_write; 									//Set to read by default
	payload.bytes = i2c_load->bytes;											//given the number of bytes
	payload.data = i2c_load->data;												//Passed the data from the i2c
	payload.cmd = i2c_load->cmd;												//given the hold command

//	location = MS;																//Most significant byte first
	payload.current_state = initialize;											//Set to first state
	payload.peripheral->CMD = I2C_CMD_START;									//Send the command to start
	payload.peripheral->TXDATA = (i2c_load->device_address << 1) | write; 		//Send the device address to transmit buffer (write is 0)
																//~(0x01)
}

/***************************************************************************//**
 * @brief
 *	 function that the I2Cx interrupt handler will call upon receiving the I2C ACK interrupt
 ******************************************************************/
void I2C_ACK()
{
	switch(payload.current_state) //depending on the value of the local variable
	{
		case initialize:
			payload.peripheral->TXDATA = payload.cmd;							//Transmit buffer data register is set
																				//payload.peripheral->CMD = payload.cmd (I2C_CMD_NO_HOLD)
			payload.current_state = send_measure_cmd;							//The state is changed
			break;																//And we exit the function
		case send_measure_cmd:
			payload.peripheral->CMD = I2C_CMD_START;							//Send the start command
			payload.peripheral->TXDATA = (payload.device_address << 1) | read;  //Transmit buffer data register is sent the address to read from
			payload.current_state = send_read_cmd;								//Change the state
			break;
		case send_read_cmd:
			payload.current_state = receive_data;								//The state is changed and we exit the function
			break;
		case receive_data:
			EFM_ASSERT(false);													//Otherwise, something has gone wrong
			break;
		case end_process:
			EFM_ASSERT(false);
			break;
		default:
			EFM_ASSERT(false);
			break;
	}
}

/***************************************************************************//**
 * @brief
 *	 function that the I2Cx interrupt handler will call upon receiving the I2C NACK interrupt
 ******************************************************************/
void I2C_NACK()
{
	switch(payload.current_state) //depending on the value of the local variable
	{
		case initialize:
			EFM_ASSERT(false);
			break;
		case send_measure_cmd:
			EFM_ASSERT(false);
			break;
		case send_read_cmd:
			payload.peripheral->CMD = I2C_CMD_START;							//Repeated start
			payload.peripheral->TXDATA = (payload.device_address << 1) | read;	//Transmit buffer data register is sent the address to read from again
			//payload.current_state = receive_data;								//We don't do this b/c we want to stay in this state until data ready
			break;
		case receive_data:
			EFM_ASSERT(false);
			break;
		case end_process:
			EFM_ASSERT(false);
			break;
		default:
			EFM_ASSERT(false);
			break;
	}
}

/***************************************************************************//**
 * @brief
 *	 function that the I2Cx interrupt handler will call upon receiving the RXDATAV interrupt
 ******************************************************************/
void I2C_RXDATAV()
{
	switch(payload.current_state) //depending on the value of the local variable
	{
		case initialize:
			EFM_ASSERT(false);
			break;
		case send_measure_cmd:
			EFM_ASSERT(false);
			break;
		case send_read_cmd:
			EFM_ASSERT(false);
			break;
		case receive_data:
			payload.bytes -= 1;													//After receiving the first byte, we decrement to 1 left, then 0
			if(payload.bytes > 0)												//If on the most significant byte
			{
				*payload.data = payload.peripheral->RXDATA << 8;				//We take in data from the receive buffer data register
				payload.peripheral->CMD = I2C_CMD_ACK;							//Then we send back an acknowledgement
			}
			else																//If on the least significant byte
			{
				*payload.data |= payload.peripheral->RXDATA;					//We take in data from the receive buffer data register
				payload.peripheral->CMD = I2C_CMD_NACK;							//Then we send back a no acknowledgement
				payload.peripheral->CMD = I2C_CMD_STOP;							//and a stop
				payload.current_state = end_process;							//and change states
			}
			break;
		case end_process:
			EFM_ASSERT(false);
			break;
		default:
			EFM_ASSERT(false);
			break;
	}
}

/***************************************************************************//**
 * @brief
 *	 function that the I2Cx interrupt handler will call upon receiving the MSTOP interrupt
 ******************************************************************/
void I2C_MSTOP()
{
	switch(payload.current_state) //depending on the value of the local variable
	{
		case initialize:
			EFM_ASSERT(false);
			break;
		case send_measure_cmd:
			EFM_ASSERT(false);
			break;
		case send_read_cmd:
			EFM_ASSERT(false);
			break;
		case receive_data:
			EFM_ASSERT(false);
			break;
		case end_process:
			add_scheduled_event(SI7021_READ_EVT);	//adding the event to the scheduler
			sleep_unblock_mode(I2C_EM_BLOCK);		//Going back to sleep
			payload.current_state = initialize;		//Reseting the state to the beginning
			break;
		default:
			EFM_ASSERT(false);
			break;
	}
}

