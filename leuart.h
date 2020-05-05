/**
 * @file leuart.h
 * @author Connor Humiston
 * @date 3/17/20
 * @brief Defines LEUART parameters, initialization structures, and LE UART functions
 *
 */

#ifndef SRC_HEADER_FILES_LEUART_H
#define SRC_HEADER_FILES_LEUART_H

//***********************************************************************************
// Include files
//***********************************************************************************
#include "em_leuart.h"
#include "sleep_routines.h"

//***********************************************************************************
// defined files
//***********************************************************************************
#define LEUART_EM		EM3
#define STARTF_CHAR 	(uint8_t) '#'
#define SIGF_CHAR 		(uint8_t) '?'
//#define FAHRENHEIT_CHAR	(uint8_t) 'F'
//#define CELCIUS_CHAR	(uint8_t) 'C'

//***********************************************************************************
// global variables
//***********************************************************************************
//LEUART_OPEN_STRUCT is used by an application module to open an LEUART peripheral
typedef struct
{
	uint32_t					baudrate;
	LEUART_Databits_TypeDef		databits;
	LEUART_Enable_TypeDef		enable;
	LEUART_Parity_TypeDef 		parity;
	uint32_t 					refFreq; //
	LEUART_Stopbits_TypeDef		stopbits;
	bool						rxblocken;
	bool						sfubrx;
	bool						startframe_en;
	char						startframe;
	bool						sigframe_en;
	char						sigframe;
	uint32_t					rx_loc;
	uint32_t					rx_pin_en;
	uint32_t					tx_loc;
	uint32_t					tx_pin_en;
	bool						rx_en;
	bool						tx_en;
	uint32_t					rx_done_evt;
	uint32_t					tx_done_evt;
} LEUART_OPEN_STRUCT;

typedef enum
{
	begin,
	transmit,
	transmit_done,
//	end
} leuart_tx_states;

typedef enum
{
	idle,
	start,
	receive,
	done,
} leuart_rx_states;

typedef struct
{
	leuart_tx_states		state;				//current state in transmit machine
	uint32_t				count;				//for counting down the number of characters left
	char 					str[80];			//the array string being transmitted
	uint32_t				index;				//what index we are on (increasing) to assemble word (transmit index)
	volatile bool 			txbusy;				//reports if the transmitter is busy or not

	leuart_rx_states		rx_state;			//current receiving state
	char 					received_str[80];	//the array string being received
	uint32_t				rx_count;			//the receiver index/count
	char					startf;				//the start frame character
	char					sigf;				//the sig frame character for receiving
	volatile bool			rxbusy;				//reports if the receiver is busy or not
} LEUART_PAYLOAD_STRUCT;


//***********************************************************************************
// function prototypes
//***********************************************************************************
void leuart_open(LEUART_TypeDef *leuart, LEUART_OPEN_STRUCT *leuart_settings);
void leuart_rx_test(LEUART_TypeDef *leuart);
void LEUART0_IRQHandler(void);
void leuart_start(LEUART_TypeDef *leuart, char *string, uint32_t string_len);
void TXBL_Interrupt(void);
void TXC_Interrupt(void);
bool leuart_tx_busy(LEUART_TypeDef *leuart);
bool leuart_rx_busy(LEUART_TypeDef *leuart);

uint32_t leuart_status(LEUART_TypeDef *leuart);
void leuart_cmd_write(LEUART_TypeDef *leuart, uint32_t cmd_update);
void leuart_if_reset(LEUART_TypeDef *leuart);
void leuart_app_transmit_byte(LEUART_TypeDef *leuart, uint8_t data_out);
uint8_t leuart_app_receive_byte(LEUART_TypeDef *leuart);
void rx_str_copy(char *destination);

#endif
