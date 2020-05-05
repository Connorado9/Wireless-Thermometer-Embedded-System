/**
 * @file ble.c
 * @author Keith Graham
 * @date November 28th, 2019
 * @brief Contains all the functions to interface the application with the HM-18
 *   BLE module and the LEUART driver
 * @details
 *   This module contains all the functions to interface the application layer
 *  with the HM-18 Bluetooth module.  The application does not have the
 *  responsibility of knowing the physical resources required, how to
 *  configure, or interface to the Bluetooth resource including the LEUART
 *  driver that communicates with the HM-18 BLE module.
 *
 */

//***********************************************************************************
// Include files
//***********************************************************************************
#include "ble.h"
#include <string.h>

//***********************************************************************************
// defined files
//***********************************************************************************


//***********************************************************************************
// private variables
//***********************************************************************************
static CIRC_TEST_STRUCT test_struct;
static BLE_CIRCULAR_BUF ble_cbuf;

//***********************************************************************************
// Private functions
//***********************************************************************************
//static bool ble_circ_pop(bool test);
static uint8_t ble_circ_space(void);
static void update_circ_wrtindex(BLE_CIRCULAR_BUF *index_struct, uint32_t update_by);
static void update_circ_readindex(BLE_CIRCULAR_BUF *index_struct, uint32_t update_by);

//***********************************************************************************
// Global functions
//***********************************************************************************


/***************************************************************************//**
 * @brief
 *	 This function initializes the bluetooth low energy module.
 * @details
 *	 ble_open initializes a local LEUART_OPEN structure to fill in parameters, and call the leuart open to initialize the UART for the bleutooth module.
 * @param[in] tx_event
 * 	 tx_event acts as an input for the macro expansion representing the transmitter's unique event bit
 * @param[in] rx_event
 *   rx event acts as an input for the receiver's unique bit
 ******************************************************************/
void ble_open(uint32_t tx_event, uint32_t rx_event)
{
	LEUART_OPEN_STRUCT local_leuart;
	local_leuart.baudrate = HM18_BAUDRATE;
	local_leuart.databits = HM18_DATABITS;
	local_leuart.enable = HM18_ENABLE;
	local_leuart.parity = HM18_PARITY;
	local_leuart.refFreq = HM18_REFFREQ;
	local_leuart.stopbits = HM18_STOPBITS;
	local_leuart.rx_loc = LEUART0_RX_ROUTE;
	local_leuart.tx_loc = LEUART0_TX_ROUTE;
	local_leuart.rx_pin_en = true;
	local_leuart.tx_pin_en = true;
	local_leuart.tx_en = true;
	local_leuart.rx_en = true;
	local_leuart.tx_done_evt = tx_event;
	local_leuart.rx_done_evt = rx_event;
//	local_leuart.sfubrx = false;
//	local_leuart.sigframe = 0;
//	local_leuart.sigframe_en = false;
//	local_leuart.startframe = 0;
//	local_leuart.startframe_en = 0;
//	local_leuart.rxblocken = 0;

	leuart_open(HM18_LEUART0, &local_leuart);

	ble_circ_init();
}

/***************************************************************************//**
 * @brief
 *	 ble_write writes an input string to the leuart.
 * @details
 *	 This function initializes leuart for the HM18 bluetooth module, and sends the string and string length to the leuart_start function for that purpose.
 * 	 desired I2Cx peripheral, such as I2C0, I2C1, etc.
 * @note
 * 	 This function is currently specific to the HM18 bluetooth module low energy UART
 * @param[in] string
 *   The string to be sent over the leaurt
 ******************************************************************/
void ble_write(char* string)
{
	//leuart_start(HM18_LEUART0, string, strlen(string));
	ble_circ_push(string);
	ble_circ_pop(false);
}

/***************************************************************************//**
 * @brief
 *   BLE Test performs two functions.  First, it is a Test Driven Development
 *   routine to verify that the LEUART is correctly configured to communicate
 *   with the BLE HM-18 module.  Second, the input argument passed to this
 *   function will be written into the BLE module and become the new name
 *   advertised by the module while it is looking to pair.
 *
 * @details
 * 	 This global function will use polling functions provided by the LEUART
 * 	 driver for both transmit and receive to validate communications with
 * 	 the HM-18 BLE module.  For the assignment, the communication with the
 * 	 BLE module must use low energy design principles of being an interrupt
 * 	 driven state machine.
 *
 * @note
 *   For this test to run to completion, the phone most not be paired with
 *   the BLE module.  In addition for the name to be stored into the module
 *   a breakpoint must be placed at the end of the test routine and stopped
 *   at this breakpoint while in the debugger for a minimum of 5 seconds.
 *
 * @param[in] *mod_name
 *   The name that will be written to the HM-18 BLE module to identify it
 *   while it is advertising over Bluetooth Low Energy.
 *
 * @return
 *   Returns bool true if successfully passed through the tests in this
 *   function.
 ******************************************************************************/
bool ble_test(char *mod_name)
{
	uint32_t str_len;

	__disable_irq();
	// This test will limit the test to the proper setup of the LEUART
	// peripheral, routing of the signals to the proper pins, pin
	// configuration, and transmit/reception verification.  The test
	// will communicate with the BLE module using polling routines
	// instead of interrupts.

	// How is polling different than using interrupts?
	// ANSWER: 				In interrupt, the device notifies the CPU that it needs servicing whereas,
	//						in polling CPU repeatedly checks whether a device needs servicing.

	// How do interrupts benefit the system for low energy operation?
	// ANSWER:  			Interrupts benefit low energy operation becuase they don't constantly poll and can sleep in the meantime.

	// How do interrupts benefit the system that has multiple tasks?
	// ANSWER: 				Interrupts benefit multiple tasks because each task can have its own interrupt.

	// First, you will need to review the DSD HM10 datasheet to determine
	// what the default strings to write data to the BLE module and the
	// expected return statement from the BLE module to test / verify the
	// correct response

	//The break_str is used to tell the BLE module to end a Bluetooth connection such as with your phone.
	//The ok_str is the result sent from the BLE module
	// to the micro-controller if there was not active BLE connection at the time
	// the break command was sent to the BLE module.
	// Replace the break_str "" with the command to break or end a BLE connection
	// Replace the ok_str "" with the result that will be returned from the BLE
	//   module if there was no BLE connection
	char		break_str[80] = "AT";
	char		ok_str[80] = "OK";

	// output_str will be the string that will program a name to the BLE module.
	// From the DSD HM10 datasheet, what is the command to program a name into
	// the BLE module?
	// The  output_str will be a string concatenation of the DSD HM10 command
	// and the input argument sent to the ble_test() function
	// Replace the otput_str "" with the command to change the program name
	// Replace the result_str "" with the first part of the expected result
	//  the backend of the expected response will be concatenated with the
	//  input argument
	char		output_str[80] = "AT+NAME";
	char		result_str[80] = "OK+Set:";

	// To program the name into your module, you must reset the module after you
	// have sent the command to update the modules name.  What is the DSD HM18
	// name to reset the module?
	// Replace the reset_str "" with the command to reset the module
	// Replace the reset_result_str "" with the expected BLE module response to
	//  to the reset command
	char		reset_str[80] = "AT+RESET";
	char		reset_result_str[80] = "OK+RESET";
	char		return_str[80];

	bool		success;
	bool		rx_disabled, rx_en, tx_en;
	uint32_t	status;

	// These are the routines that will build up the entire command and response
	// of programming the name into the BLE module.  Concatenating the command or
	// response with the input argument name
	strcat(output_str, mod_name);
	strcat(result_str, mod_name);

	// The test routine must not alter the function of the configuration of the
	// LEUART driver, but requires certain functionality to insure the logical test
	// of writing and reading to the DSD HM10 module.  The following c-lines of code
	// save the current state of the LEUART driver that will be used later to
	// re-instate the LEUART configuration

	status = leuart_status(HM18_LEUART0);
	if (status & LEUART_STATUS_RXBLOCK)
	{
		rx_disabled = true;
		// Enabling, unblocking, the receiving of data from the LEUART RX port
		leuart_cmd_write(HM18_LEUART0, LEUART_CMD_RXBLOCKDIS);
	}
	else rx_disabled = false;
	if (status & LEUART_STATUS_RXENS)
	{
		rx_en = true;
	}
	else
	{
		rx_en = false;
		// Enabling the receiving of data from the RX port
		leuart_cmd_write(HM18_LEUART0, LEUART_CMD_RXEN);
		while (!(leuart_status(HM18_LEUART0) & LEUART_STATUS_RXENS)); /////////////////////
	}

	if (status & LEUART_STATUS_TXENS)
	{
		tx_en = true;
	}
	else
	{
		// Enabling the transmission of data to the TX port
		leuart_cmd_write(HM18_LEUART0, LEUART_CMD_TXEN);
		while (!(leuart_status(HM18_LEUART0) & LEUART_STATUS_TXENS));
		tx_en = false;
	}
//	leuart_cmd_write(HM10_LEUART0, (LEUART_CMD_CLEARRX | LEUART_CMD_CLEARTX));

	// This sequence of instructions is sending the break ble connection
	// to the DSD HM10 module.
	// Why is this command required if you want to change the name of the
	// DSD HM18 module?
	// ANSWER:  			To program the name into your module, you must reset the module after you have sent the command to update the modules name.
	//						The break ble connection is required in case you connect to a different device to avoid calibration errors
	str_len = strlen(break_str);
	for (int i = 0; i < str_len; i++)
	{
		leuart_app_transmit_byte(HM18_LEUART0, break_str[i]);
	}

	// What will the ble module response back to this command if there is
	// a current ble connection?
	// ANSWER: 				Its response will be OK+LOSS.
	str_len = strlen(ok_str);
	for (int i = 0; i < str_len; i++)
	{
		return_str[i] = leuart_app_receive_byte(HM18_LEUART0);
		if (ok_str[i] != return_str[i])
		{
				EFM_ASSERT(false);;
		}
	}

	// This sequence of code will be writing or programming the name of
	// the module to the DSD HM10
	str_len = strlen(output_str);
	for (int i = 0; i < str_len; i++){
		leuart_app_transmit_byte(HM18_LEUART0, output_str[i]);
	}

	// Here will be the check on the response back from the DSD HM10 on the
	// programming of its name
	str_len = strlen(result_str);
	for (int i = 0; i < str_len; i++){
		return_str[i] = leuart_app_receive_byte(HM18_LEUART0);
		if (result_str[i] != return_str[i])
		{
				EFM_ASSERT(false);;
		}
	}

	// It is now time to send the command to RESET the DSD HM10 module
	str_len = strlen(reset_str);
	for (int i = 0; i < str_len; i++)
	{
		leuart_app_transmit_byte(HM18_LEUART0, reset_str[i]);
	}

	// After sending the command to RESET, the DSD HM10 will send a response
	// back to the micro-controller
	str_len = strlen(reset_result_str);
	for (int i = 0; i < str_len; i++)
	{
		return_str[i] = leuart_app_receive_byte(HM18_LEUART0);
		if (reset_result_str[i] != return_str[i])
		{
				EFM_ASSERT(false);;
		}
	}

	// After the test and programming have been completed, the original
	// state of the LEUART must be restored
	if (!rx_en) leuart_cmd_write(HM18_LEUART0, LEUART_CMD_RXDIS);
	if (rx_disabled) leuart_cmd_write(HM18_LEUART0, LEUART_CMD_RXBLOCKEN);
	if (!tx_en) leuart_cmd_write(HM18_LEUART0, LEUART_CMD_TXDIS);
	leuart_if_reset(HM18_LEUART0);

	success = true;

	__enable_irq();
	return success;
}

/***************************************************************************//**
 * @brief
 *   Circular Buffer test is a Test Driven Development routine to verify
 *   that the circular buffer is functioning correctly
 * @note
 *   The test checks that:
 *   - One string can be added to the circular buffer and be successfully popped off
 *   - More than one string can be added to the circular buffer and each individual
 *     string then can be successfully popped off the circular buffer
 *   - The buffer can act in a circular nature by having the strings added to loop
 *     around the buffer and successfully be popped off
 ******************************************************************************/
void circular_buff_test(void)
{
	 bool buff_empty;
	 int test1_len = 50;
	 int test2_len = 25;
	 int test3_len = 5;

	 // Why this 0 initialize of read and write pointer?
	 // Student Response:
	 // 	The read and write pointers should be initialized to the first element of the circular buffer
	 ble_cbuf.read_ptr = 0;
	 ble_cbuf.write_ptr = 0;


	 // Why do none of these test strings contain a 0?
	 // Student Response:
	 //		You cannot send a string with length 0;
	 for (int i = 0;i < test1_len; i++){
		 test_struct.test_str[0][i] = i+1;
	 }
	 for (int i = 0;i < test2_len; i++){
		 test_struct.test_str[1][i] = i + 20;
	 }
	 for (int i = 0;i < test3_len; i++){
		 test_struct.test_str[2][i] = i +  35;
	 }

	 // Why is there only one push to the circular buffer at this stage of the test
	 // Student Response:
	 //		This stage tests whether or not the circular buffer is initialized and functioning properly Before attempting to push multiple strings
	 ble_circ_push(&test_struct.test_str[0][0]);

	 // Why is the expected buff_empty test = false?
	 // Student Response:
	 //		the buffer isn't empty;
	 //		buff_empty is set to the output of ble_circ_pop which pops something off the buffer to be sent to the bluetooth module,
	 //		so it should be false unless there was an unsuccessful pop in which case the pop function is updating the result_str[]
	 //		from the private CIRC_TEST_STRUCT to be evaluate by this function instead of sending the data to the LEUART
	 buff_empty = ble_circ_pop(CIRC_TEST);
	 EFM_ASSERT(buff_empty == false);
	 for (int i = 0; i < test1_len; i++){
		 EFM_ASSERT(test_struct.test_str[0][i] == test_struct.result_str[i]);
	 }

	 // What does this next push on the circular buffer test?
	 // Student Response:
	 // 	This push tests to see if a second element works on the circular buffer.
	 ble_circ_push(&test_struct.test_str[1][0]);

	 // What does this next push on the circular buffer test?
	 // Student Response:
	 //		This push tests a third element on the buffer
	 ble_circ_push(&test_struct.test_str[2][0]);


	 // Why is the expected buff_empty test = false?
	 // Student Response:
	 //		the buffer isn't empty, and should have popped the string to the LEUART so pop should return false;
	 //
	 //		buff_empty is set to the output of ble_circ_pop which pops something off the buffer to be sent to the bluetooth module,
	 //		so it should be false again unless there was an unsuccessful pop in which case the pop function is updating the result_str[]
	 //		from the private CIRC_TEST_STRUCT to be evaluate by this function instead of sending the data to the LEUART
	 buff_empty = ble_circ_pop(CIRC_TEST);
	 EFM_ASSERT(buff_empty == false);
	 for (int i = 0; i < test2_len; i++){
		 EFM_ASSERT(test_struct.test_str[1][i] == test_struct.result_str[i]);
	 }

	 // Why is the expected buff_empty test = false?
	 // Student Response:
	 //		the buffer isn't empty, and should have popped the string to the LEUART so pop should return false;
	 //
	 //		buff_empty is set to the output of ble_circ_pop which pops something off the buffer to be sent to the bluetooth module,
	 //		so it should be false again unless there was an unsuccessful pop in which case the pop function is updating the result_str[]
	 //		from the private CIRC_TEST_STRUCT to be evaluate by this function instead of sending the data to the LEUART
	 buff_empty = ble_circ_pop(CIRC_TEST);
	 EFM_ASSERT(buff_empty == false);
	 for (int i = 0; i < test3_len; i++){
		 EFM_ASSERT(test_struct.test_str[2][i] == test_struct.result_str[i]);
	 }

	 // Using these three writes and pops to the circular buffer, what other test
	 // could we develop to better test out the circular buffer?
	 // Student Response:
	 // 	The circular buffer could also be tested by reading its contents directly and what strings it sends to the leuart by comparing the
	 //		contents after multiple pushes/pops to the expected result. We could also make sure we can completely fill the buffer.


	 // Why is the expected buff_empty test = true?
	 // Student Response:
	 //		The buffer is now empty/idle so pop should return true;
	 //		After popping once more, all elements should popped off the now empty buffer
	 buff_empty = ble_circ_pop(CIRC_TEST);
	 EFM_ASSERT(buff_empty == true);
	 ble_write("\nPassed Circular Buffer Test...\n");
}

/***************************************************************************//**
 * @brief
 *   Initializes the ble's circular buffer to the correct size and pointer values
 ******************************************************************************/
void ble_circ_init(void)
{
	//ble_cbuf.cbuf;
	ble_cbuf.size = CSIZE;
	ble_cbuf.size_mask = CSIZE;
	ble_cbuf.write_ptr = 0;
	ble_cbuf.read_ptr = 0;
}

/***************************************************************************//**
 * @brief
 *   This function pushes/adds a packet(string) onto the buffer when called
 * @note
 *   This function should only add a packet onto the circular buffer if there is room to add the entire packet
 *   If there is no room to add a packet, you code should call EFM_ASSERT(false) which will halt your program
 *   As stated earlier, your end application will need to define how to handle situation when your circular buffer is about to become overwritten
 * @param[in] *string
 *	 The string packet being added to the buffer
 ******************************************************************************/
void ble_circ_push(char *string)
{
	int length = strlen(string);
	int index = 0;

	//Check if room to add the packet
	if(ble_cbuf.size < length)
	{
		EFM_ASSERT(false); //If no room to add packet
	}
	ble_cbuf.cbuf[ble_cbuf.write_ptr] = length;
	update_circ_wrtindex(&ble_cbuf, 1);

	//Push characters in the string at the proper index to the buffer and update the write index and index each time
	while(index != length)
	{
		ble_cbuf.cbuf[ble_cbuf.write_ptr] = string[index];
		update_circ_wrtindex(&ble_cbuf, 1);
		index++;
	}

	//Update the buffer size
	__disable_irq();
	ble_cbuf.size = ble_circ_space() - (length+1);
	__enable_irq();
}

/***************************************************************************//**
 * @brief
 *	 Pops off a complete packet from the circular buffer
 * @note
 *	  This function also must strip off the header so that just the data string
 *	  can be sent to the LEUART for transmission using the leaurt_start() function
 * @param[in] test
 *	  If test is true, the pop function will not send data to the LEUART.
 *	  If test is false, this function sends the string to the BLE module via the leuart_start() function
 * @return
 *	  This function returns true if the LEUART is busy or full, and false otherwise
 ******************************************************************************/
bool ble_circ_pop(bool test)
{
	//If the LEUART is in the middle of a string transmission, we should just exit
	if(!(leuart_status(HM18_LEUART0) & LEUART_STATUS_TXIDLE))
	{
		return true;
	}
	if(ble_cbuf.size == CSIZE) //if size limit reached, exit
	{
		return true;
	}

	//Reading the header that contains string's length and updates the read index
	int length = ble_cbuf.cbuf[ble_cbuf.read_ptr];
	update_circ_readindex(&ble_cbuf, 1);

	char string_array[length+1];
	int index = 0;

	//Pop a character from the buffer to the string array and update the indices each time
	while(index != length)
	{
		string_array[index] = ble_cbuf.cbuf[ble_cbuf.read_ptr];
		update_circ_readindex(&ble_cbuf, 1);
		index++;
	}

	string_array[length] = '\0';

	//If the input argument test is true, the pop function should not send the data to leuart_start(),
	//but instead update the result_str[] from the private CIRC_TEST_STRUCT to be evaluate by the circular buffer test function
	if(test == true)
	{
		for(int i = 0; i < length; i++)
		{
			test_struct.result_str[i] = string_array[i];
		}
	}
	else //If the input argument is false, the routine should send the string to the BLE module via the leuart_start() function
	{
		leuart_start(HM18_LEUART0, string_array, length);
		while(leuart_tx_busy(LEUART0)); //wait until done transmitting
	}

	//Update the buffer size
	__disable_irq();
	ble_cbuf.size = (ble_circ_space() + length + 1);
	__enable_irq();

	return false;
}

/***************************************************************************//**
 * @brief
 *   Returns space available in the circular buffer
 * @return
 *	 This functions returns an 8 bit integer containing the space available in the buffer
 ******************************************************************************/
static uint8_t ble_circ_space(void)
{
	//return (CSIZE - 1) - (ble_cbuf.write_ptr - ble_cbuf.read_ptr); //the amount of space available in the buffer is the size minus the space already used
	return ble_cbuf.size;

	//return ble_cbuf.size_mask;
	//return (ble_cbuf.size - 1) - (ble_cbuf.write_ptr - ble_cbuf.read_ptr);
	//return (ble_cbuf.write_ptr - ble_cbuf.read_ptr) & (ble_cbuf.size - 1);
}

/***************************************************************************//**
 * @brief
 *   Updates the circular buffer write index value
 * @note
 *   This function will be called by the push a packet onto the circular buffer function if there is enough room to add a packet
 *   The write index variable is used to specify the index location in the circular buffer array where the next write will occur
 * @param[in] *index_struct
 *   The structure containing the circular buffer being updated
 * @param[in] *update_by
 *	 This parameter contains the integer describing how much the index should be updated by
 ******************************************************************************/
static void update_circ_wrtindex(BLE_CIRCULAR_BUF *index_struct, uint32_t update_by)
{
	index_struct->write_ptr = (index_struct->write_ptr + update_by)%64;
}

/***************************************************************************//**
 * @brief
 *	 Updates the buffer read index value
 * @note
 *   This function will be called by the pop off a packet from your circular buffer function once a packet has been read from the buffer
 *   The read index variable is used to specify the index location in the circular buffer array where the next read will occur
 * @param[in] *index_struct
 *   The structure containing the circular buffer being updated
 * @param[in] *update_by
 *   The integer describing how much the index should be updated by
 ******************************************************************************/
static void update_circ_readindex(BLE_CIRCULAR_BUF *index_struct, uint32_t update_by)
{
	index_struct->read_ptr = (index_struct->read_ptr + update_by)%64;
}


