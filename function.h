//***********************************************************************************
// Copyright 2017, Zksiot Development Ltd.
// Created by Linjie, 2017.08.08
// MCU:	MSP430F5529
// OS: TI-RTOS
// Project:
// File name: function.h
// Description: various function routine head.
//***********************************************************************************

#ifndef __ZKSIOT_FUNCTION_H__
#define __ZKSIOT_FUNCTION_H__

#ifdef LITTLE_ENDIAN
// little endian
#define	HIBYTE(w)		(((uint8_t *)&w)[1])
#define	LOBYTE(w)		(((uint8_t *)&w)[0])
#define	HIWORD(w)		(((uint16_t *)&w)[1])
#define	LOWORD(w)		(((uint16_t *)&w)[0])
#else
// 	big endian
#define	HIBYTE(w)		(((uint8_t*)&w)[0])
#define	LOBYTE(w)		(((uint8_t*)&w)[1])
#define	HIWORD(w)		(((uint16_t*)&w)[0])
#define	LOWORD(w)		(((uint16_t*)&w)[1])
#endif

#define sizeofarray(x)  (sizeof(x)/sizeof(x[0]))

#define PROTOCOL_TOKEN              0x7e
#define PROTOCOL_TRANSFER           0x7d


#define CRC_SEED                    0xffff
#define CRC_POLYNOMIAL              0x1021
#define CRC_POLYNOMIAL_REVERS       0x8408

typedef enum {
    ES_SUCCESS = 0,
    ES_ERROR = 1
} ErrorStatus;

typedef struct {
    uint8_t *objData;
    uint16_t objSize;
    uint16_t objNumber;
    uint32_t front;
    uint32_t rear;
} QueueDef;


extern uint16_t Protocol_escape(uint8_t *pObj, uint8_t *pSou, uint16_t length);
extern uint16_t Protocol_recover_escape(uint8_t *pObj, uint8_t *pSou, uint16_t length);
extern uint16_t CRC16(uint8_t *pData,  uint16_t length);
extern uint8_t CheckCode8(uint8_t *pData,  uint16_t length);
extern ErrorStatus EnQueue(QueueDef *queue, uint8_t *obj);
extern ErrorStatus DeQueue(QueueDef *queue, uint8_t *obj);

#endif	/* __ZKSIOT_FUNCTION_H__ */
