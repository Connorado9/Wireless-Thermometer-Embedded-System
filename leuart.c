/**
 * @file leuart.c
 * @author Connor Humiston
 * @date November 27th, 2019
 * @brief LEUART driver Contains all the functions of the LEUART peripheral
 * @details
 *  This module contains all the functions to support the driver's state
 *  machine to transmit a string of data across the LEUART bus.  There are
 *  additional functions to support the Test Driven Development test that
 *  is used to validate the basic set up of the LEUART peripheral.  The
 *  TDD test for this class assumes that the LEUART is connected to the HM-18
 *  BLE module.  These TDD support functions could be used for any TDD test
 *  to validate the correct setup of the LEUART.
 */

//***********************************************************************************
// Include files
//***********************************************************************************

//** Standard Library includes
#include <string.h>
#include <stdio.h>

//** Silicon Labs include files
#include "em_gpio.h"
#include "em_cmu.h"

//** Developer/user include files
#include "leuart.h"
#include "scheduler.h"
#include "app.h"

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// private variables
//***********************************************************************************
uint32_t	rx_done_evt;
uint32_t	tx_done_evt;
uint32_t	str_ptr;
uint32_t	max_len;
char		output_str[80];
bool		leuart0_tx_busy;
LEUART_PAYLOAD_STRUCT lePayload;

//***********************************************************************************
// Private functions
//***********************************************************************************
static void STARTF_Interrupt(void);
static void RXDATAV_Interrupt(void);
static void SIGF_Interrupt(void);

//***********************************************************************************
// Global functions
//***********************************************************************************

/***************************************************************************//**
 * @brief
 *		This function sets up the low energy UART.
 * @details
 *		leuart_open enables the correct clock, initializes the UART, routes the peripheral to the proper board pins, and enables interrupts at the CPU level
 * @note
 *		Only LEUART0 is expected to be set up at this time
 * @param[in] leuart
 *		This is the LEUART that needs setting up
 * @param[in] leuart_settings
 * 		This is the open structure that passes in UART parameters to set up
 ******************************************************************************/
void leuart_open(LEUART_TypeDef *leuart, LEUART_OPEN_STRUCT *leuart_settings)
{
	//Determining which LEUART peripheral is attempting to be opened, and then enabling its clock
	if(leuart == LEUART0)
	{
		CMU_ClockEnable(cmuClock_LEUART0, true);
	}
	else
	{
		EFM_ASSERT(false);
	}

	//Verify that the clock tree has been enabled correctly
	int temp = leuart->STARTFRAME;
	leuart->STARTFRAME = ~temp; //!
	//Polling to make sure synchronization is complete
	while(leuart->SYNCBUSY); //Checks that the CMU and LEUART clock signals have synchronized in LEUART's lower frequency domain
	EFM_ASSERT(temp != leuart->STARTFRAME);
	leuart->STARTFRAME = temp;
	while(leuart->SYNCBUSY);

	//Initialize the LEUART peripheral
	LEUART_Init_TypeDef leuart_temp;
	leuart_temp.baudrate = leuart_settings->baudrate;
	leuart_temp.databits = leuart_settings->databits;
	leuart_temp.enable = leuart_settings->enable;
	leuart_temp.parity = leuart_settings->parity;
	leuart_temp.refFreq = leuart_settings->refFreq;
	leuart_temp.stopbits = leuart_settings->stopbits;

	LEUART_Reset(leuart);
	LEUART_Init(leuart, &leuart_temp);
	while(leuart->SYNCBUSY); //stall after init
	leuart->CTRL &= ~LEUART_CTRL_AUTOTRI;

	//After initializing the peripheral you must route TX and RX to proper Gecko GPIO pins
	leuart->ROUTELOC0 = leuart_settings->rx_loc | leuart_settings->tx_loc;
	leuart->ROUTEPEN = (leuart_settings->rx_pin_en * LEUART_ROUTEPEN_RXPEN) | (leuart_settings->tx_pin_en * LEUART_ROUTEPEN_TXPEN);

	//After setting up and before enabling LEUART0, clear TX transmit and RX receive buffers
	leuart->CMD = LEUART_CMD_CLEARRX | LEUART_CMD_CLEARTX;

	//Enable the LEUART
	//LEUART_Enable(leuart, leuartEnable);

	//EFM ASSERT reading the value of RXEN and TXEN from the STATUS register
	if(leuart_settings->enable == (leuartEnableRx & leuartEnable))// checking if receiver enabled //LEUART_CMD_RXEN
	{
		EFM_ASSERT((leuart->STATUS | LEUART_STATUS_TXENS) & (leuart->STATUS | LEUART_STATUS_RXENS));
	}
	else
	{
		EFM_ASSERT(~((leuart->STATUS | LEUART_STATUS_TXENS) & (leuart->STATUS | LEUART_STATUS_RXENS)));
	}
	if(leuart_settings->enable == (leuartEnableTx & leuartEnable)) // checking if transmitter enabled //LEUART_CMD_TXEN
	{
		EFM_ASSERT((leuart->STATUS | LEUART_STATUS_TXENS) & (leuart->STATUS | LEUART_STATUS_RXENS));
	}
	else
	{
		EFM_ASSERT(~((leuart->STATUS | LEUART_STATUS_TXENS) & (leuart->STATUS | LEUART_STATUS_RXENS)));
	}
	//Poll LEUARTx->STATUS register for these signals to be asserted

	tx_done_evt = LEUART_TX_EVT;
	rx_done_evt = LEUART_RX_EVT;
	lePayload.txbusy = false;

	//FOR RECEIVING DATA//
	char start = '#';
	char sig = '?';
	while(leuart->SYNCBUSY);					//Wait for synchronization
	LEUART0->STARTFRAME = start;				//Set the STARTF register with the proper char
	while(leuart->SYNCBUSY);
	LEUART0->SIGFRAME = sig;					//Set the SIGF register with the proper char
	while(leuart->SYNCBUSY);
	leuart->CMD |= LEUART_CMD_RXBLOCKEN;		//enable RXBLOCK to block incoming data
	while(leuart->SYNCBUSY);
	leuart->CTRL |= LEUART_CTRL_SFUBRX;			//enable defined start frame
	//Block sleep here instead of leuart_start

	//setting up for receiving
	lePayload.rxbusy = false;					//The rx is not busy yet
	lePayload.startf = STARTF_CHAR;
	lePayload.sigf = SIGF_CHAR;
	lePayload.rx_state = idle; 					//ready for receiving
	sleep_block_mode(LEUART_EM);				//Block sleep for receiving now

	//scheduled_leuart0_tx_done_evt(); //moved below

	//Clear interrupts
	LEUART_IntClear(leuart, LEUART_IFC_TXC | LEUART_IFC_STARTF | LEUART_IFC_SIGF);
	//Enable the interrupts
	leuart->IEN |= LEUART_IEN_STARTF;			//Enable STARTF interrupt by default
	LEUART0->IEN &= ~LEUART_IEN_SIGF;			//disable SIGF & RXDATAV interrupts
	LEUART0->IEN &= ~LEUART_IEN_RXDATAV;

	//Enabling the interrupts at CPU level
	if(leuart == LEUART0)
	{
		NVIC_EnableIRQ(LEUART0_IRQn);
	}
	else
	{
		EFM_ASSERT(false);
	}

	//scheduled_leuart0_tx_done_evt();
}


/***************************************************************************//**
 * @brief
 *		This function handles interrupts for the low energy UART
 * @details
 *		Interrupts are disabled to prevent interrupts from occurring when handling an interrupt.
 *		Then, the interrupt that was raised is determined with the flag register.
 *		Depending on the interrupt, the correct function is called to handle the change further.
 ******************************************************************************/
void LEUART0_IRQHandler(void)
{
	__disable_irq(); 	//temporarily disable interrupts
	uint32_t int_flag = (LEUART0->IF & LEUART0->IEN); 	//Locally store the source interrupts by ANDing flag and enable to see only ints enabled
	LEUART0->IFC = int_flag;	//Clearing the current interrupts with Interrupt Flag Clear Register so they can occur again

	if(int_flag & LEUART_IF_TXBL)
	{
		TXBL_Interrupt();
	}
	if(int_flag & LEUART_IF_TXC)
	{
		TXC_Interrupt();
	}
	if(int_flag & LEUART_IF_STARTF)
	{
		STARTF_Interrupt();
	}
	if(int_flag & LEUART_IF_RXDATAV)
	{
		RXDATAV_Interrupt();
	}
	if(int_flag & LEUART_IF_SIGF)
	{
		SIGF_Interrupt();
	}

	__enable_irq(); //Re-enable interrupts
}


/***************************************************************************//**
 * @brief
 *		This function starts the low energy UART, and sets up for transmission and receiving.
 * @details
 *		leuart_start blocks the Gecko from sleeping, sets the first states, fills variables, clears possibly expired TXC interrupts, and enables the TXBL interrupt.
 * @param[in] leuart
 *		This is the LEUART that needs set up and started
 * @param[in] string
 * 		The string to be sent over the UART
 * @param[in] string_len
 * 		The length of the string to be sent over the UART
 ******************************************************************************/
void leuart_start(LEUART_TypeDef *leuart, char *string, uint32_t string_len)
{
	sleep_block_mode(LEUART_EM);

	//transmission setup
	lePayload.txbusy = true;
	lePayload.state = transmit;
	lePayload.count = string_len;
	lePayload.index = 0;
	strcpy(lePayload.str, string);				//Copy the passed string into the payload structure
	LEUART_IntClear(leuart, LEUART_IFC_TXC);	//Clear existing interrupts
	leuart->IEN |= LEUART_IEN_TXBL; 			//only start with TXBL enabled
}


/***************************************************************************//**
 * @brief
 *   LEUART test is a Test Driven Development routine to verify that the LEUART
 *   is correctly configured to receive data.
 * @details
 *	 This TDD completes various checks to ensure proper LEUART receiving.
 *	 First, it verifies that transmission and receiving are enabled in the STATUS register.
 *	 Next, it ensures that start and signal frames are working,
 *	 and that data sent before on accident does not affect anything
 *	 Finally, it sends a series of characters to simulate an actual test,
 *	 and compares the result with the string that was received.
 *	 An EFM_ASSERT will occur if these items are not properly functioning.
 * @note
 * 	 The start frame is # and the signal frame is ?.
 * 	 Also, this function uses loopback mode so the transmission data loops around to be received.
 * 	 Notice that the function does not test for proper parsing
 * 	 of the start and signal frame as this is done outside the leuart.
 * @param[in] *leaurt
 *   The low energy UART that will be tested (LEUART0)
 ******************************************************************************/
void leuart_rx_test(LEUART_TypeDef *leuart)
{
	//Using loopback, when we transmit data it comes back to the receive port
	leuart->CTRL |= LEUART_CTRL_LOOPBK; //enable loopback TX->RX

	//Check to make sure TX and RX are enabled
	EFM_ASSERT(leuart->STATUS & LEUART_STATUS_TXENS);
	EFM_ASSERT(leuart->STATUS & LEUART_STATUS_RXENS);

	//The start frame and signal frames will be signaled with # and ?
	char startf = STARTF_CHAR;
	char sigf = SIGF_CHAR;

	__disable_irq();
	//So first we will write to the tx data register, and ensure that the RX data register is empty since it is currently blocked
	leuart->TXDATA = 'a'; 					//Since RXBLOCK should be enabled, we should not receive the a
	while(!(leuart->IF & LEUART_IF_TXC));	//As soon as TXC bit set in IF register, we know a has been transmitted fully
	leuart->IFC |= LEUART_IFC_TXC; 			//Clear the interrupt
	EFM_ASSERT(!(leuart->IF & LEUART_IF_RXDATAV));

	//Next, we check if a start frame can be received which will automatically unblock the data
	leuart->TXDATA = startf;				//transmit the start frame
	while(!(leuart->IF & LEUART_IF_TXC));	//wait for startf to transmit
	while(!(leuart->IF & LEUART_IF_RXDATAV));//Expecting to have received the data so we wait for that, If we pass this point, we know the startf unblocked rx
	EFM_ASSERT(leuart->IF & LEUART_IF_STARTF);//see if we get the STARTF interrupt or not
	EFM_ASSERT(leuart->RXDATA == startf);	//The startf should be the only thing in RXDATA (no a)
	leuart->IFC = LEUART_IFC_TXC;			//Clear the interrupt

	//Next, we check if sig frame is working
	leuart->TXDATA = sigf;					//Send signal frame
	while(!(leuart->IF & LEUART_IF_TXC));
	while(!(leuart->IF & LEUART_IF_RXDATAV));
	EFM_ASSERT(leuart->IF & LEUART_IF_SIGF);
	EFM_ASSERT(leuart->RXDATA == sigf);	//Check for sigf
	leuart->IFC = LEUART_IFC_TXC;

	//Reset everything
	while(leuart->SYNCBUSY);
	leuart->CMD |= LEUART_CMD_RXBLOCKEN;		//enable RXBLOCK to block incoming data again
	while(leuart->SYNCBUSY);
	leuart->CTRL |= LEUART_CTRL_SFUBRX;			//enable defined start frame
	LEUART_IntClear(leuart, LEUART_IFC_TXC | LEUART_IFC_STARTF | LEUART_IFC_SIGF); //clears interrupts

	__enable_irq();

	//Finally we put it all together and test the state machine with a series of strings
	char *test_str = "Hello#Test4U?\nRXTestPass...";
	char *result_str = "#Test4U?";
	leuart_start(leuart, test_str, strlen(test_str)); //transmit actual message
	//while(leuart->SYNCBUSY);
	while(leuart_tx_busy(LEUART0));
	while(leuart_rx_busy(LEUART0));
	//Check that the message received was the same as intended
	EFM_ASSERT(strcmp(lePayload.received_str, result_str) == 0);

	leuart->CTRL &= ~LEUART_CTRL_LOOPBK; //disable loopback
}


/***************************************************************************//**
 * @brief
 *		This function handles the TXBL interrupt and is called in the LEUART IRQ handler
 * @details
 *		A TXBL interrupt indicates the level of the transmit buffer; set when the transmit buffer is empty, and cleared when it is full.
 *		This function's case statements determine the current state, and deal with each accordingly.
 ******************************************************************************/
void TXBL_Interrupt(void)
{
	switch(lePayload.state) //depending on the value of the local variable
	{
		case begin:
			EFM_ASSERT(false);
			break;
		case transmit:
			if(lePayload.count > 0)
			{
				lePayload.count--;
				LEUART0->TXDATA = (uint8_t) lePayload.str[lePayload.index];
				lePayload.index++;
			}
			if(lePayload.count == 0)
			{
				lePayload.state = transmit_done;
				LEUART0->IEN &= ~LEUART_IEN_TXBL;
				LEUART0->IEN |= LEUART_IEN_TXC;
			}
			break;
		case transmit_done:
			EFM_ASSERT(false);
			break;
		default:
			EFM_ASSERT(false);
			break;
	}
}

/***************************************************************************//**
 * @brief
 *		This function handles the TXC interrupt and is called in the LEUART IRQ handler
 * @details
 *		A TXC interrupt signals that a transmission has been completed and no more data is available in the transmit buffer.
 *		This function's case statements determine the current state, and deal with each accordingly.
 ******************************************************************************/
void TXC_Interrupt(void)
{
	switch(lePayload.state) //depending on the value of the local variable
	{
		case begin:
			EFM_ASSERT(false);
			break;
		case transmit:
			EFM_ASSERT(false);
			break;
		case transmit_done:
			sleep_unblock_mode(LEUART_EM);
			add_scheduled_event(tx_done_evt);
			lePayload.txbusy = false;
			//lePayload.state = end;
			break;
		default:
			EFM_ASSERT(false);
			break;
	}
}


/***************************************************************************//**
 * @brief
 *	 This function handles the STARTF interrupt when receiving data on the LEUART
 * @details
 *	 A STARTF interrupt signals that there will be incoming characters (an incoming start frame)
 * @note
 * 	 The receiver must be enabled to observe a start frame.
 * 	 The receive buffer will discard chars until the start frame.
 ******************************************************************************/
static void STARTF_Interrupt(void)
{
	switch(lePayload.rx_state)
	{
		case idle:
			if(lePayload.rxbusy == false)
			{
				lePayload.rxbusy = true;				//receiver is busy
				lePayload.rx_count = 0; 				//reset the receiver index
				LEUART0->IEN |= LEUART_IEN_RXDATAV; 	//enable RXDATAV
				LEUART0->IEN |= LEUART_IEN_SIGF; 		//enable SIGF
			}
			else
			{
				EFM_ASSERT(false);						//ensure that the last transmission is finished
				//For example, if you type #Te#st? you would run into an EFM Assert below
			}
			lePayload.rx_state = start; //was start
			break;
		case start:
			EFM_ASSERT(false);
			break;
		case receive:
			EFM_ASSERT(false);
			break;
		case done:
			EFM_ASSERT(false);
			break;
		default:
			EFM_ASSERT(false);
			break;
	}
}


/***************************************************************************//**
 * @brief
 *	 This function handles the RXDATAV interrupt when receiving data on the LEUART
 * @details
 *   The RXDATAV interrupt signals that data has become available in the receive buffer
 * @note
 *   The receiver must be enabled for signal frames to be detected
 ******************************************************************************/
static void RXDATAV_Interrupt(void)
{
	switch(lePayload.rx_state)
	{
		case idle:
			EFM_ASSERT(false);
			break;
		case start:
			lePayload.rx_state = receive;				//Using this state to simply change states skips adding the STARTF character to the string
			break;
		case receive:
			//if((lePayload.rx_count == 0) && (strcmp(LEUART0)))//(LEUART0->RXDATA == lePayload.startf)){}//(lePayload.received_str[lePayload.rx_count] == lePayload.startf)){} //if we are on the first character and it is # startf, we dont add that to the str array
			//else if(lePayload.received_str[lePayload.rx_count] == lePayload.sigf)
			//{
			//	lePayload.received_str[lePayload.rx_count] = '\0';				//add a null character to the place where the sigf character would have went
			//	lePayload.rx_count++;			   								//increment count still
			//}
			//else
			//{
				lePayload.received_str[lePayload.rx_count] = LEUART0->RXDATA;	//read the data
				lePayload.rx_count++; 											//increment the count
			//}
			break; 																//the state will change with the SIGF interrupt
		case done:
			EFM_ASSERT(false);
			break;
		default:
			EFM_ASSERT(false);
			break;
	}
}


/***************************************************************************//**
 * @brief
 *	 This function handles the SIGF interrupt when receiving data on the LEUART
 * @details
 *	 The SIGF interrupt signals the end of a multi-frame message transmitted to the LEUART
 * @note
 *   The receiver must be enabled for signal frames to be detected
 ******************************************************************************/
static void SIGF_Interrupt(void)
{
	switch(lePayload.rx_state)
	{
		case idle:
			EFM_ASSERT(false);
			break;
		case start:
			EFM_ASSERT(false);
			break;
		case receive:
			//lePayload.rx_state = done; //When we get the SIGF interrupt at the end of the receiving state, we should switch to done and wait for the incoming RXDATAV interrupt
			lePayload.received_str[lePayload.rx_count] = '\0';			//Edit the data string by replacing sigf character with '\0'
			lePayload.rx_count++;
			LEUART0->IEN &= ~LEUART_IEN_SIGF;								//disable SIGF & RXDATAV interrupts
			LEUART0->IEN &= ~LEUART_IEN_RXDATAV;
			LEUART0->CMD |= LEUART_CMD_RXBLOCKEN;							//enable RXBLOCK
			add_scheduled_event(LEUART_RX_EVT);								//set rx_done_evt
			lePayload.rx_state = idle;

			lePayload.rxbusy = false;
			break;
		case done:
			EFM_ASSERT(false);
			break;
		default:
			EFM_ASSERT(false);
			break;
	}
}


/***************************************************************************//**
 * @brief
 *   Returns whether the leuart is is in the middle of transmitting or not
 * @param[in] *leuart
 *   Defines the LEUART peripheral being checked
 * @return
 * 	 Returns true if leuart is transmitting and false if not
 ******************************************************************************/
bool leuart_tx_busy(LEUART_TypeDef *leuart)
{
	return lePayload.txbusy;
}


/***************************************************************************//**
 * @brief
 *   Returns whether the leuart is is in the middle of receiving or not
 * @param[in] *leuart
 *   Defines the LEUART peripheral being checked
 * @return
 * 	 Returns true if leuart is receiving and false if not
 ******************************************************************************/
bool leuart_rx_busy(LEUART_TypeDef *leuart)
{
	return lePayload.rxbusy;
}


/***************************************************************************//**
 * @brief
 *   LEUART STATUS function returns the STATUS of the peripheral for the
 *   TDD test
 *
 * @details
 * 	 This function enables the LEUART STATUS register to be provided to
 * 	 a function outside this .c module.
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 * @return
 * 	 Returns the STATUS register value as an uint32_t value
 *
 ******************************************************************************/
uint32_t leuart_status(LEUART_TypeDef *leuart)
{
	uint32_t	status_reg;
	status_reg = leuart->STATUS;
	return status_reg;
}


/***************************************************************************//**
 * @brief
 *   LEUART CMD Write sends a command to the CMD register
 *
 * @details
 * 	 This function is used by the TDD test function to program the LEUART
 * 	 for the TDD tests.
 *
 * @note
 *   Before exiting this function to update  the CMD register, it must
 *   perform a SYNCBUSY while loop to ensure that the CMD has by synchronized
 *   to the lower frequency LEUART domain.
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 * @param[in] cmd_update
 * 	 The value to write into the CMD register
 *
 ******************************************************************************/
void leuart_cmd_write(LEUART_TypeDef *leuart, uint32_t cmd_update){

	leuart->CMD = cmd_update;
	while(leuart->SYNCBUSY);
}


/***************************************************************************//**
 * @brief
 *   LEUART IF Reset resets all interrupt flag bits that can be cleared
 *   through the Interrupt Flag Clear register
 *
 * @details
 * 	 This function is used by the TDD test program to clear interrupts before
 * 	 the TDD tests and to reset the LEUART interrupts before the TDD
 * 	 exits
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 ******************************************************************************/
void leuart_if_reset(LEUART_TypeDef *leuart)
{
	leuart->IFC = 0xffffffff;
}


/***************************************************************************//**
 * @brief
 *   LEUART App Transmit Byte transmits a byte for the LEUART TDD test
 *
 * @details
 * 	 The BLE module will respond to AT commands if the BLE module is not
 * 	 connected to the phone app.  To validate the minimal functionality
 * 	 of the LEUART peripheral, write and reads to the LEUART will be
 * 	 performed by polling and not interrupts.
 *
 * @note
 *   In polling a transmit byte, a while statement checking for the TXBL
 *   bit in the Interrupt Flag register is required before writing the
 *   TXDATA register.
 *
 * @param[in] *leuart
 *   Defines the LEUART peripheral to access.
 *
 * @param[in] data_out
 *   Byte to be transmitted by the LEUART peripheral
 *
 ******************************************************************************/
void leuart_app_transmit_byte(LEUART_TypeDef *leuart, uint8_t data_out)
{
	while (!(leuart->IF & LEUART_IF_TXBL));
	leuart->TXDATA = data_out;
}


/***************************************************************************//**
 * @brief
 *   LEUART App Receive Byte polls a receive byte for the LEUART TDD test
 *
 * @details
 * 	 The BLE module will respond to AT commands if the BLE module is not
 * 	 connected to the phone app.  To validate the minimal functionality
 * 	 of the LEUART peripheral, write and reads to the LEUART will be
 * 	 performed by polling and not interrupts.
 *
 * @note
 *   In polling a receive byte, a while statement checking for the RXDATAV
 *   bit in the Interrupt Flag register is required before reading the
 *   RXDATA register.
 *
 * @param[in] leuart
 *   Defines the LEUART peripheral to access.
 *
 * @return
 * 	 Returns the byte read from the LEUART peripheral
 *
 ******************************************************************************/
uint8_t leuart_app_receive_byte(LEUART_TypeDef *leuart)
{
	uint8_t leuart_data;
	while (!(leuart->IF & LEUART_IF_RXDATAV));
	leuart_data = leuart->RXDATA;
	return leuart_data;
}

/***************************************************************************//**
 * @brief
 *   This function, when called, copies the LEUART's received string into the destination address
 * @param[in] destination
 *   This input represents the destination address where the rx string will be copied to
 ******************************************************************************/
void rx_str_copy(char *destination)
{
	memcpy(destination, lePayload.received_str, strlen(lePayload.received_str));
}
