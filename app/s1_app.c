/*
* @Author: zxt
* @Date:   2018-03-09 11:13:28
* @Last Modified by:   zxt
* @Last Modified time: 2018-09-18 10:19:51
*/
#include "../general.h"



/***** Defines *****/


/***** Type declarations *****/


/***** Variable declarations *****/
uint32_t configModeTimeCnt;          // the unit is sec



/***** Prototypes *****/



/***** Function definitions *****/



//***********************************************************************************
// brief:   
// 
// parameter: 
//***********************************************************************************
void S1HwInit(void)
{
    LedInit();
    
    KeyInit();
    KeyRegister(SystemKeyEventPostIsr, KEY_0_SHORT_PRESS);
    KeyRegister(SystemLongKeyEventPostIsr, KEY_0_LONG_PRESS);
    KeyRegister(SystemDoubleKeyEventPostIsr, KEY_0_DOUBLE_PRESS);

    Spi_init();

    I2c_init();

    Flash_init();

    configModeTimeCnt = 0;
    g_rSysConfigInfo.sensorModule[0] = SEN_TYPE_SHT2X;
}




//***********************************************************************************
// brief:the node short key application
// 
// parameter: 
//***********************************************************************************
void S1ShortKeyApp(void)
{
    switch(deviceMode)
    {
        case DEVICES_ON_MODE:
        Led_ctrl(LED_B, 1, 200 * CLOCK_UNIT_MS, 1);
        break;

        case DEVICES_OFF_MODE:
        Led_ctrl(LED_R, 1, 200 * CLOCK_UNIT_MS, 1);
        break;

        case DEVICES_CONFIG_MODE:
        Led_ctrl(LED_G, 1, 200 * CLOCK_UNIT_MS, 1);
        break;

    }
}

//***********************************************************************************
// brief:the node long key application
// 
// parameter: 
//***********************************************************************************
void S1LongKeyApp(void)
{
    switch(deviceMode)
    {
        case DEVICES_ON_MODE:
        case DEVICES_CONFIG_MODE:
        S1Sleep();
        g_rSysConfigInfo.sysState.wtd_restarts &= (0xFFFF^STATUS_POWERON);
        Led_ctrl2(LED_R, 1, 200 * CLOCK_UNIT_MS, 800 * CLOCK_UNIT_MS, 3);
        g_rSysConfigInfo.rtc = Rtc_get_calendar();
        Flash_store_config();
        Task_sleep(3000 * CLOCK_UNIT_MS);
        SysCtrlSystemReset();
        break;

        case DEVICES_OFF_MODE:
        if(Battery_get_voltage() <= g_rSysConfigInfo.batLowVol)
        {
            Led_ctrl(LED_R, 1, 200 * CLOCK_UNIT_MS, 1);
        }
        else
        {
            Led_ctrl2(LED_B, 1, 200 * CLOCK_UNIT_MS, 800 * CLOCK_UNIT_MS, 3);
            g_rSysConfigInfo.sysState.wtd_restarts |= STATUS_POWERON;
            S1Wakeup();
            Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
        }
        break;
    }
}


//***********************************************************************************
// brief:the node long key application
// 
// parameter: 
//***********************************************************************************
void S1DoubleKeyApp(void)
{
    switch(deviceMode)
    {
        case DEVICES_ON_MODE:
        case DEVICES_CONFIG_MODE:
#if   defined(SUPPORT_BOARD_OLD_S1) || defined(SUPPORT_BOARD_OLD_S2S_1)
            OldS1NodeApp_stopSendSensorData();
#endif
        // enter DEVICES_CONFIG_MODE, clear radio tx buf and send the config parameter to config deceive
        // if(RadioStatueRead() == RADIOSTATUS_TRANSMITTING)
        NodeStrategyReset();
        deviceMode                      = DEVICES_CONFIG_MODE;
        configModeTimeCnt = 0;
        NodeUploadOffectClear();
        //RadioModeSet(RADIOMODE_RECEIVEPORT);
        SetRadioDstAddr(CONFIG_DECEIVE_ID_DEFAULT);
#if   defined(SUPPORT_BOARD_OLD_S1) || defined(SUPPORT_BOARD_OLD_S2S_1)
        RadioSwitchingUserRate();
#endif
        NodeStrategyStop();
        RadioEventPost(RADIO_EVT_SEND_CONFIG);
        Led_ctrl2(LED_G, 1, 200 * CLOCK_UNIT_MS, 800 * CLOCK_UNIT_MS, 3);
        break;
    }
}


void S1AppRtcProcess(void)
{
	if(deviceMode == DEVICES_CONFIG_MODE && RADIOMODE_UPGRADE != RadioModeGet())
    {
        configModeTimeCnt++;
        if(configModeTimeCnt >= 120)
        {
            NodeStrategyBuffClear();
            RadioModeSet(RADIOMODE_SENDPORT);
            NodeStartBroadcast();
            NodeBroadcasting();
            NodeStrategyStart();
            deviceMode = DEVICES_ON_MODE;
        }
    }

}

extern void WdtResetCb(uintptr_t handle);



//***********************************************************************************
// brief:   S1 wakeup enable the rtc / wdt and the node function
// 
// parameter: 
//***********************************************************************************
void S1Wakeup(void)
{
    deviceMode = DEVICES_ON_MODE;
    RtcStart();

#ifdef  SUPPORT_WATCHDOG
    WdtInit(WdtResetCb);
#endif
    NodeWakeup();
}


//***********************************************************************************
// brief:   S1 sleep enable the rtc / wdt and the node function
// 
// parameter: 
//***********************************************************************************
void S1Sleep(void)
{
    deviceMode = DEVICES_OFF_MODE;

#if  !defined(SUPPORT_BOARD_OLD_S1) || !defined(SUPPORT_BOARD_OLD_S2S_1)
    RtcStop();
#endif
    NodeSleep();
}
