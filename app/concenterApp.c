/*
* @Author: justfortest
* @Date:   2017-12-28 10:09:45
* @Last Modified by:   zxt
* @Last Modified time: 2020-06-10 14:24:26
*/
#include "../general.h"






#define     NODE_SETTING_CMD_LENGTH    10

#define     CONCENTER_RADIO_MONITOR_CNT_MAX     60



/***** Type declarations *****/
typedef struct 
{
    uint32_t channelDispath;
    uint32_t monitorCnt;
    uint32_t noRecCnt;
    bool     synTimeFlag;    // 0: unsyntime; 1: synchron time
}concenter_para_t;





/***** Variable declarations *****/

concenter_para_t concenterParameter;




extflash_queue_s extflashWriteQ;
/***** Prototypes *****/




/***** Function definitions *****/



//***********************************************************************************
// brief:   init the concenter upload timer
// 
// parameter: 
//***********************************************************************************
void ConcenterAppInit(void)
{

    concenterParameter.channelDispath = 0;
    concenterParameter.monitorCnt     = 0;
    concenterParameter.noRecCnt       = 0;

    concenterParameter.synTimeFlag     = false;

    ExtflashRingQueueInit(&extflashWriteQ);



// *******************************for test*************************
    // g_rSysConfigInfo.DeviceId[0] = (uint8_t)((DECEIVE_ID_DEFAULT>>24)&0xff);
    // g_rSysConfigInfo.DeviceId[1] = (uint8_t)((DECEIVE_ID_DEFAULT>>16)&0xff);
    // g_rSysConfigInfo.DeviceId[2] = (uint8_t)((DECEIVE_ID_DEFAULT>>8)&0xff);
    // g_rSysConfigInfo.DeviceId[3] = (uint8_t)((DECEIVE_ID_DEFAULT)&0xff);;

    // g_rSysConfigInfo.customId[0] = (uint8_t)(CUSTOM_ID_DEFAULT >> 8);
    // g_rSysConfigInfo.customId[1] = (uint8_t)(CUSTOM_ID_DEFAULT);
// *******************************

    SetRadioSrcAddr(RADIO_CONTROLER_ADDRESS);
}




//***********************************************************************************
// brief:   start the upload timer
// 
// parameter: 
//***********************************************************************************
void ConcenterUploadStart(void)
{
}


//***********************************************************************************
// brief:   stop the upload timer
// 
// parameter: 
//***********************************************************************************
void ConcenterUploadStop(void)
{
}

//***********************************************************************************
// brief:   set the upload timer period
// 
// parameter
// period:  the uint is ms
//***********************************************************************************
void ConcenterUploadPeriodSet(uint32_t period)
{

}


//***********************************************************************************
// brief:  store the sensor data to queue
// 
// parameter: 
//***********************************************************************************
bool ConcenterSensorDataSaveToQueue(uint8_t *dataP, uint8_t length)
{
    if(ExtflashRingQueuePush(&extflashWriteQ, dataP) == true)
    {
        Event_post(systemAppEvtHandle, SYSTEMAPP_EVT_STORE_CONCENTER);
        return true;
    }
    else
    {
//        Event_post(systemAppEvtHandle, SYSTEMAPP_EVT_STORE_CONCENTER);
        return false;
    }
}

//***********************************************************************************
// brief:  save the sensor data to extflash
// 
// parameter: 
//***********************************************************************************
void ConcenterSensorDataSave(void)
{
    uint8_t dataP[FLASH_SENSOR_DATA_SIZE];
    Calendar calendar;


    if(ExtflashRingQueuePoll(&extflashWriteQ, dataP) == true)
    {
        calendar = Rtc_get_calendar();
        dataP[dataP[0]+1] = 0xfe;
        dataP[dataP[0]+2] = 0xfe;
        dataP[dataP[0]+3] = TransHexToBcd((uint8_t)(calendar.Year - 2000));
        dataP[dataP[0]+4] = TransHexToBcd((uint8_t)(calendar.Month));
        dataP[dataP[0]+5] = TransHexToBcd((uint8_t)(calendar.DayOfMonth));
        dataP[dataP[0]+6] = TransHexToBcd((uint8_t)(calendar.Hours));
        dataP[dataP[0]+7] = TransHexToBcd((uint8_t)(calendar.Minutes));
        dataP[dataP[0]+8] = TransHexToBcd((uint8_t)(calendar.Seconds));
        dataP[0]         += 8;

        Flash_store_sensor_data(dataP, FLASH_SENSOR_DATA_SIZE);
        Event_post(systemAppEvtHandle, SYSTEMAPP_EVT_STORE_CONCENTER);
    }
#ifdef  SUPPORT_DEVICED_STATE_UPLOAD
    if(g_bNeedUploadRecord){
        Flash_store_devices_state(TYPE_RECORD_START);
        g_bNeedUploadRecord = 0;
    }       
#endif
}


//***********************************************************************************
// brief:   concenter upload the data to the internet
// 
// parameter: 
//***********************************************************************************
void ConcenterSensorDataUpload(void)
{
    uint8_t     data[24];
    uint32_t    dataItems;
    uint8_t     offsetUnit;
    //reverse the buf to other command
    offsetUnit = 0;
    dataItems  = Flash_get_unupload_items();
    
    while(dataItems)
    {
        Flash_load_sensor_data_by_offset(data, 22, dataItems);

        // upload the data to network 
        
        InterfaceSend(data, 32);
        dataItems--;
        offsetUnit++;
        
    }
}       


//***********************************************************************************
// brief: seach the Node parameter setting table to updata the specify node parasetting
// 
// parameter: 
//***********************************************************************************
void ConcenterUpdataNodeSetting(uint32_t srcAddr, uint32_t dstAddr)
{
    // search the table to updata the parameter setting
    // 
}

//***********************************************************************************
// brief: seach the Node parameter setting table to update the table updata flag
// 
// parameter: 
//***********************************************************************************
void ConcenterNodeSettingSuccess(uint32_t srcAddr, uint32_t dstAddr)
{
    // search the table to clear the special node parameter seeting 
}



//***********************************************************************************
// brief:   make the concenter board into sleep mode
// 
// parameter: 
//***********************************************************************************
void ConcenterSleep(void)
{

    concenterParameter.synTimeFlag  = false;
    RadioDisable();

}

//***********************************************************************************
// brief:   make the Concenter board into work mode
// 
// parameter: 
//***********************************************************************************
void ConcenterWakeup(void)
{
    RadioModeSet(RADIOMODE_RECEIVEPORT);
}


//***********************************************************************************
// brief:   Init the board as the config deceive
// 
// parameter: 
//***********************************************************************************
void ConcenterConfigDeceiveInit(void)
{

    deviceMode = DEVICES_CONFIG_MODE;
    EasyLink_setCtrl(EasyLink_Ctrl_AsyncRx_TimeOut, 0);
    RadioModeSet(RADIOMODE_RECEIVEPORT);
    InterfaceEnable();
}




//***********************************************************************************
// brief:save the config to internal flash
// 
// parameter: 
//***********************************************************************************
void ConcenterStoreConfig(void)
{
    Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
}

//***********************************************************************************
// brief:
// 
// parameter: 
//***********************************************************************************
void ConcenterTimeSychronization(Calendar *currentTime)
{
    Rtc_set_calendar(currentTime);
    concenterParameter.synTimeFlag  = 1;
}

//***********************************************************************************
// brief:
// 
// parameter: 
//***********************************************************************************
uint8_t ConcenterReadSynTimeFlag(void)
{
    return concenterParameter.synTimeFlag;
}



//***********************************************************************************
// brief:
// 
// parameter: 
//***********************************************************************************
void ConcenterRadioMonitorClear(void)
{
    concenterParameter.monitorCnt = 0;
    concenterParameter.noRecCnt   = 0;
}


//***********************************************************************************
// brief:the concenter rtc process
// 
// parameter: 
//***********************************************************************************
void ConcenterRtcProcess(void)
{
    concenterParameter.monitorCnt++;
    if(concenterParameter.monitorCnt >= g_rSysConfigInfo.uploadPeriod+10)
    {
        Sys_event_post(SYSTEMAPP_EVT_CONCENTER_MONITER);
    }

}

//***********************************************************************************
// brief:the concenter send the radio data in order to reset radio state
// 
// parameter: 
//***********************************************************************************
void ConcenterResetRadioState(void)
{
    concenterParameter.noRecCnt++;
    Flash_log("NoRec\n");
    if(concenterParameter.noRecCnt > 3){
        SystemResetAndSaveRtc();
        concenterParameter.noRecCnt = 0;
    }

    ConcenterRadioMonitorClear();
    RadioAbort();
    // RadioSetRxMode();
    SetRadioDstAddr(0xdadadada);
    g_rSysConfigInfo.sysState.lora_send_errors ++;
    Flash_store_config();
    RadioEventPost(RADIO_EVT_SEND_CONFIG);
}



