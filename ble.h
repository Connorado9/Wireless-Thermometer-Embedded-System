/**
 * @file ble.h
 * @author Connor Humiston
 * @date 3/17/20
 * @brief Defines HM18 parameters, LEUART routing info, and bluetooth low energy functions
 *
 */

#ifndef SRC_HEADER_FILES_BLE_H
#define SRC_HEADER_FILES_BLE_H


//***********************************************************************************
// Include files & Standard Libraries
//***********************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include "leuart.h"


//***********************************************************************************
// defined files
//***********************************************************************************
#define HM18_LEUART0		LEUART0
#define HM18_BAUDRATE		9600
#define	HM18_DATABITS		leuartDatabits8  			//8 data bits not 9
#define HM18_ENABLE			leuartEnable				//enable blt transmitter
#define HM18_PARITY			leuartNoParity 				//No parity not even odd
#define HM18_REFFREQ		0 // use reference clock
#define HM18_STOPBITS		leuartStopbits1 			//1 stop bit

#define LEUART0_TX_ROUTE	LEUART_ROUTELOC0_TXLOC_LOC18
#define LEUART0_RX_ROUTE	LEUART_ROUTELOC0_RXLOC_LOC18

#define CIRC_TEST_SIZE 		3
#define CIRC_TEST 			true
#define CIRC_OPER 			false
#define CSIZE 				64

//***********************************************************************************
// global variables
//***********************************************************************************
typedef struct
{
	char 		cbuf[CSIZE];
	uint8_t		size_mask;
	uint32_t	size;
	uint32_t	read_ptr;
	uint32_t	write_ptr;
} BLE_CIRCULAR_BUF;

typedef struct
{
	char test_str[CIRC_TEST_SIZE][CSIZE];
	char result_str[CSIZE];
} CIRC_TEST_STRUCT;

//***********************************************************************************
// function prototypes
//***********************************************************************************
void ble_open(uint32_t tx_event, uint32_t rx_event);
void ble_write(char *string);
bool ble_test(char *mod_name);

void circular_buff_test(void);
void ble_circ_init(void);
void ble_circ_push(char *string);
bool ble_circ_pop(bool test);

#endif
