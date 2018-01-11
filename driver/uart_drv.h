//***********************************************************************************
// Copyright 2017, Zksiot Development Ltd.
// Created by Linjie, 2017.08.08
// MCU:	MSP430F5529
// OS: TI-RTOS
// Project:
// File name: uart_drv.h
// Description: uart process routine head.
//***********************************************************************************

#ifndef __ZKSIOT_UART_DRV_H__
#define __ZKSIOT_UART_DRV_H__


/*!
 *  @def    CC1310_LAUNCHXL_UARTName
 *  @brief  Enum of UARTs
 */
typedef enum CC1310_LAUNCHXL_UARTName {
    CC1310_LAUNCHXL_UART0   = 0,
    CC1310_LAUNCHXL_UARTCOUNT,
} CC1310_LAUNCHXL_UARTName;
/*****************************************************/







#define UART_BUFF_SIZE           256

typedef void (*UART_CB_T)(uint8_t *dataP, uint8_t len);

typedef enum {
    UART_0,
    UART_MAX
} UART_PORT;

typedef struct {
    uint8_t buff[UART_BUFF_SIZE];
    uint8_t length;
} UartRxData_t;



extern UartRxData_t     uart0RxData;
extern UartRxData_t     uart0IsrRxData;
extern UartRxData_t     uart0TxData;



#define	UartSoftInit(baud, cb)		UartHwInit(UART_0, baud, cb)

#define	UartSend(bufp, count)		UartSendDatas(UART_0, bufp, count)


void UartHwInit(UART_PORT uartPort, uint32_t baudrate, UART_CB_T callback);

void UartClose(UART_PORT uartPort);

void UartSendDatas(UART_PORT uartPort, uint8_t *buf, uint8_t count);

void Uart_send_burst_data(UART_PORT uartPort, uint8_t *pData, uint16_t length);

void Uart_send_string(UART_PORT uartPort, uint8_t *string);


#endif	/* __ZKSIOT_UART_DRV_H__ */

