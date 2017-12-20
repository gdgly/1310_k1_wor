//***********************************************************************************
// Copyright 2017, Zksiot Development Ltd.
// Created by Linjie, 2017.08.08
// MCU:	MSP430F5529
// OS: TI-RTOS
// Project:
// File name: key_proc.h
// Description: key process routine head.
//***********************************************************************************

#ifndef __ZKSIOT_KEY_PROC_H__
#define __ZKSIOT_KEY_PROC_H__

// Define key action
typedef enum {
    KEY_NONE,
    KEY_0_PRESS,
    KEY_1_PRESS,
    KEY_01_PRESS,
    KEY_ACTION_MAX
} KEY_ACTION;

// Define key code.
#define _VK_NULL                0xff
#define _VK_POWER               0x01
#define _VK_SELECT              0x02
#define _VK_MENU                0x03
#define _VK_MENU_DOWN           0x04


// Define key pressed time, unit is 10ms.
#define TIME_KEY_NEW            3
#define TIME_KEY_DOUBLE         50
#define TIME_KEY0_LONG          400
#define TIME_KEY1_LONG          200

// Key state define.
#define KEY_PRESSED             0
#define KEY_RELEASE             1

typedef struct {
    // Key hold on time count.
    uint16_t holdTime;
    // Key double press time count.
    uint16_t doublePressTime;
    // Key short press flag.
    uint8_t shortPress;
    // Key double press flag.
    uint8_t doublePress;
    // key action.
    uint8_t action;
    // key code.
    uint8_t code;
} KeyTask_t;



void Key_init(void);
uint8_t Key_get(void);

#endif	/* __ZKSIOT_KEY_PROC_H__ */
