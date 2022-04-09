
/*
 Software Uart Implementation For Stm32
 Special Thanks to Liyanboy75 for providing
 details of implementation
 https://github.com/liyanboy74
*/
#ifndef SW_UART_H
#define SW_UART_H

#include <stdint.h> // for int*_t
#include "main.h"   // For HAL_Functions

#define NUM_SW_UARTS    1       // Max 8

#define SW_UART_TX_BUFF_SIZE    32
#define SW_UART_RX_BUFF_SIZE    64


typedef enum {
    SWUART_OK,
    SW_UART_ERR
} Soft_Uart_state_e;

typedef struct {
    uint8_t     Tx[SW_UART_TX_BUFF_SIZE];
    uint8_t     Rx[SW_UART_RX_BUFF_SIZE];
} Soft_Uart_Buffer_t;

typedef struct {
    __IO uint8_t        TxNCompleted;
    uint8_t			    TxEnable;
	uint8_t			    RxEnable;
	uint8_t 		    TxBitShift,TxBitCounter;
	uint8_t 		    RxBitShift,RxBitCounter;
	uint8_t			    TxIndex,TxSize;
	uint8_t			    RxIndex;
	Soft_Uart_Buffer_t	*Buffer;
	GPIO_TypeDef  		*TxPort;
	uint16_t 		    TxPin;
	GPIO_TypeDef  		*RxPort;
	uint16_t 	    	RxPin;      // Pin number
	uint8_t 		    RxTimingFlag;
	uint8_t 		    RxBitOffset;

} SW_UART_T;


#endif SW_UART_H