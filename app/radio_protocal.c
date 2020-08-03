/*
* @Author: justfortest
* @Date:   2017-12-26 16:36:20
* @Last Modified by:   zxt
* @Last Modified time: 2020-07-30 16:25:14
*/
#include "../general.h"

#include "../APP/nodeApp.h"
#include "../APP/systemApp.h"
#include "../APP/radio_protocal.h"
#include "../APP/concenterApp.h"
#include "../radio_app/node_strategy.h"
#include "../radio_app/radio_app.h"

extern Semaphore_Handle recAckSemHandle;

radio_protocal_t   protocalTxBuf;

uint8_t     concenterRemainderCache;
bool nodeParaSetting = 0;

uint8_t nodeSendingLog = 0;
uint32_t nodegLogCnt;


#define 		CMD_EVT_ALL		(0xFFFFFFFFFFFFFFFF)
#define 		CMD_EVENT_MAX	64
uint16_t cmdType, cmdTypeWithRespon, cmdTypeGroud;
uint64_t cmdEvent, cmdEventWithRespon, cmdEventGroud;
uint32_t groundAddr;

uint16_t sendRetryTimes;
#define         RETRY_TIMES     3


static void log_opration_record(uint8_t cmd,uint32_t deviceId,uint32_t groupId)
{
    uint8_t buff[32] = {0},index = 0;
   switch(cmd)
   {
      case RADIO_PRO_CMD_TERM_ADD_TO_GROUP:
           sprintf((char*)buff,"%s","Add group ");
           break;
      case RADIO_PRO_CMD_TERM_DELETE_FROM_GROUP:
          sprintf((char*)buff,"%s","Delete group ");
          break;
      case RADIO_PRO_CMD_TERM_TEST:
          sprintf((char*)buff,"%s","Term test ");
          break;
      case RADIO_PRO_CMD_GROUP_TEST:
          sprintf((char*)buff,"%s","Group test ");
          break;
      case RADIO_PRO_CMD_TERM_CLOSE_CTROL:
          sprintf((char*)buff,"%s","Term close ");
          break;
      case RADIO_PRO_CMD_TERM_OPEN_CTROL:
          sprintf((char*)buff,"%s","Term open ");
          break;
      case RADIO_PRO_CMD_GROUP_CLOSE_CTROL:
          sprintf((char*)buff,"%s","Group close ");
          break;
      case RADIO_PRO_CMD_GROUP_OPEN_CTROL:
          sprintf((char*)buff,"%s","Group open ");
          break;
      case RADIO_PRO_CMD_TERM_UNLOCKING:
          sprintf((char*)buff,"%s","Term unlock ");
          break;
      case RADIO_PRO_CMD_GROUP_UNLOCKING:
          sprintf((char*)buff,"%s","Group unlock ");
          break;
      case RADIO_PRO_CMD_GROUP_POWER_HIGH:
          sprintf((char*)buff,"%s","G Power high ");
          break;
      case RADIO_PRO_CMD_GROUP_POWER_MID:
          sprintf((char*)buff,"%s","G Power mid ");
          break;
      case RADIO_PRO_CMD_GROUP_POWER_LOW:
          sprintf((char*)buff,"%s","G Power low ");
          break;
      case RADIO_PRO_CMD_TERM_POWER_HIGH:
          sprintf((char*)buff,"%s","T Power high ");
          break;
      case RADIO_PRO_CMD_TERM_POWER_MID:
          sprintf((char*)buff,"%s","T Power mid ");
          break;
      case RADIO_PRO_CMD_TERM_POWER_LOW:
          sprintf((char*)buff,"%s","T Power low ");
          break;

      case RADIO_PRO_CMD_FIXED_TERM_SUBDUE_START:
           sprintf((char*)buff,"%s","Term subdue start ");
          break;
      case RADIO_PRO_CMD_FIXED_TERM_SUBDUE_STOP:
           sprintf((char*)buff,"%s","Term subdue stop ");
          break;

      case RADIO_PRO_CMD_GROUP_SUBDUE_START:
          sprintf((char*)buff,"%s","group subdue start ");
          break;
      case RADIO_PRO_CMD_GROUP_SUBDUE_STOP:
          sprintf((char*)buff,"%s","group subdue stop ");
          break;
      case RADIO_PRO_CMD_ALL_SUBDUE_START:
           sprintf((char*)buff,"%s","All subdue");
          break;

      case RADIO_PRO_CMD_OPEN_TERMINAL_PREVENT_ESCAPE:
           sprintf((char*)buff,"%s","open Term escape");
           break;
      case RADIO_PRO_CMD_CLOSE_TERMINAL_PREVENT_ESCAPE:
           sprintf((char*)buff,"%s","close Term escape");
           break;
      case RADIO_PRO_CMD_OPEN_GROUP_PREVENT_ESCAPE:
           sprintf((char*)buff,"%s","open group escape");
           break;
      case RADIO_PRO_CMD_CLOSE_GROUP_PREVENT_ESCAPE:
           sprintf((char*)buff,"%s","close group escape");
           break;

      default:
	      return;


   	}
   index = strlen((char*)buff);
#ifdef S_G
   	buff[index++] =  'T';
   	index += sprintf((char*)(buff+index),"%5d", deviceId);
   	buff[index++] =  'G';
   	index += sprintf((char*)(buff+index),"%5d", groupId);
   	buff[index++]  =  '\n';
   	Flash_log(buff);
#else
   	buff[index++] =  'T';
   	index += sprintf((char*)(buff+index),"%5d", deviceId);
   	buff[index++] =  'G';
   	index += sprintf((char*)(buff+index),"%5d", groupId);
   	buff[index++]  =  '\n';
   	buff[index++]  =  0;
   	Flash_store_sensor_data(buff, index);
   	if(Flash_get_unupload_items()> 100){
   		Flash_moveto_offset_sensor_data(1);
   	}
#endif //S_G
}

void RadioCmdProcess(uint32_t cmdType, uint32_t dstDev, uint32_t ground, uint32_t srcDev)
{
	switch(cmdType){

#ifdef S_C

		case RADIO_PRO_CMD_TERM_ADD_TO_GROUP:
			if(dstDev == GetRadioSrcAddr()){
				GroudAddrSet(ground);
				g_rSysConfigInfo.customId[0] = (uint8_t)(ground >> 24);
				g_rSysConfigInfo.customId[1] = (uint8_t)(ground >> 16);
				g_rSysConfigInfo.customId[2] = (uint8_t)(ground >> 8);
				g_rSysConfigInfo.customId[3] = (uint8_t)(ground);
				SoundEventSet(SOUND_TYPE_SET_GROUP_SUSCESS);
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_TERM_DELETE_FROM_GROUP:
			if(dstDev == GetRadioSrcAddr()){
				SoundEventSet(SOUND_TYPE_CLEAR_GROUP_SUSCESS);
				GroudAddrSet(INVALID_GROUND);
				g_rSysConfigInfo.customId[0] = (uint8_t)(INVALID_GROUND >> 24);
				g_rSysConfigInfo.customId[1] = (uint8_t)(INVALID_GROUND >> 16);
				g_rSysConfigInfo.customId[2] = (uint8_t)(INVALID_GROUND >> 8);
				g_rSysConfigInfo.customId[3] = (uint8_t)(INVALID_GROUND);
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_TERM_TEST:
			if(dstDev == GetRadioSrcAddr()){
				SoundEventSet(SOUND_TYPE_SINGLE_TEST);
				PreventiveInsertTest();
			}
		break;


		case RADIO_PRO_CMD_GROUP_TEST:
			if(ground == GroudAddrGet()){
				SoundEventSet(SOUND_TYPE_SINGLE_TEST);
				PreventiveInsertTest();
			}
		break;


		case RADIO_PRO_CMD_TERM_CLOSE_CTROL:
			if(dstDev == GetRadioSrcAddr()){
				SoundEventSet(SOUND_TYPE_CONTROL_DISABLE);
				electricshockEnable = 0;
				ElectricShockPowerDisable();
				EletricPulseSetTime_S(0);
				g_rSysConfigInfo.electricFunc &= 0xffffffff^ELE_FUNC_ENABLE_SHOCK;
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_TERM_OPEN_CTROL:
			if(dstDev == GetRadioSrcAddr()){
				SoundEventSet(SOUND_TYPE_CONTROL_ENABLE);
				electricshockEnable = 1;
				// ElectricShockPowerEnable();
				g_rSysConfigInfo.electricFunc |= ELE_FUNC_ENABLE_SHOCK;
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_GROUP_CLOSE_CTROL:
			if(ground == GroudAddrGet()){
				SoundEventSet(SOUND_TYPE_CONTROL_DISABLE);
				electricshockEnable = 0;
				ElectricShockPowerDisable();
				EletricPulseSetTime_S(0);
				g_rSysConfigInfo.electricFunc &= 0xffffffff^ELE_FUNC_ENABLE_SHOCK;
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_GROUP_OPEN_CTROL:
			if(ground == GroudAddrGet()){
				SoundEventSet(SOUND_TYPE_CONTROL_ENABLE);
				electricshockEnable = 1;
				// ElectricShockPowerEnable();
				g_rSysConfigInfo.electricFunc |= ELE_FUNC_ENABLE_SHOCK;
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_TERM_UNLOCKING:
			if(dstDev == GetRadioSrcAddr()){
				SoundEventSet(SOUND_TYPE_UNLOCK);
				eleShock_set(ELE_MOTO_ENABLE, 1);
				// Task_sleep(3000 * CLOCK_UNIT_MS);
				// eleShock_set(ELE_MOTO_ENABLE, 0);
			}
		break;


		case RADIO_PRO_CMD_GROUP_UNLOCKING:
			if(ground == GroudAddrGet()){
				SoundEventSet(SOUND_TYPE_UNLOCK);
				eleShock_set(ELE_MOTO_ENABLE, 1);
				// Task_sleep(3000 * CLOCK_UNIT_MS);
				// eleShock_set(ELE_MOTO_ENABLE, 0);
			}
		break;


		case RADIO_PRO_CMD_GROUP_POWER_HIGH:
			if(ground == GroudAddrGet()){
				SoundEventSet(SOUND_TYPE_SET_POWER_HIGH_SUCCESS);
				ElectricShockLevelSet(ELECTRIC_HIGH_LEVEL);
				g_rSysConfigInfo.electricLevel = ELECTRIC_HIGH_LEVEL;
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_GROUP_POWER_MID:
			if(ground == GroudAddrGet()){
				SoundEventSet(SOUND_TYPE_SET_POWER_MID_SUCCESS);
				ElectricShockLevelSet(ELECTRIC_MID_LEVEL);
				g_rSysConfigInfo.electricLevel = ELECTRIC_MID_LEVEL;
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_GROUP_POWER_LOW:
			if(ground == GroudAddrGet()){
				SoundEventSet(SOUND_TYPE_SET_POWER_LOW_SUCCESS);
				ElectricShockLevelSet(ELECTRIC_LOW_LEVEL);
				g_rSysConfigInfo.electricLevel = ELECTRIC_LOW_LEVEL;
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_TERM_POWER_HIGH:
			if(dstDev == GetRadioSrcAddr()){
				SoundEventSet(SOUND_TYPE_SET_POWER_HIGH_SUCCESS);
				ElectricShockLevelSet(ELECTRIC_HIGH_LEVEL);
				g_rSysConfigInfo.electricLevel = ELECTRIC_HIGH_LEVEL;
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_TERM_POWER_MID:
			if(dstDev == GetRadioSrcAddr()){
				SoundEventSet(SOUND_TYPE_SET_POWER_MID_SUCCESS);
				ElectricShockLevelSet(ELECTRIC_MID_LEVEL);
				g_rSysConfigInfo.electricLevel = ELECTRIC_MID_LEVEL;
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_TERM_POWER_LOW:
			if(dstDev == GetRadioSrcAddr()){
				SoundEventSet(SOUND_TYPE_SET_POWER_LOW_SUCCESS);
				ElectricShockLevelSet(ELECTRIC_LOW_LEVEL);
				g_rSysConfigInfo.electricLevel = ELECTRIC_LOW_LEVEL;
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_FIXED_TERM_SUBDUE_START:
			if(dstDev == GetRadioSrcAddr()){
				SoundEventSet(SOUND_TYPE_SHOCK_START);
				EletricPulseSetTime_S(ELECTRIC_SHOCK_TIME);
			}
		break;


		case RADIO_PRO_CMD_FIXED_TERM_SUBDUE_STOP:
			if(dstDev == GetRadioSrcAddr()){
				SoundEventSet(SOUND_TYPE_SHOCK_STOP);
				EletricPulseSetTime_S(0);
			}
		break;


		case RADIO_PRO_CMD_GROUP_SUBDUE_START:
			if(ground == GroudAddrGet()){
				SoundEventSet(SOUND_TYPE_SHOCK_START);
				EletricPulseSetTime_S(ELECTRIC_SHOCK_TIME);
			}
		break;


		case RADIO_PRO_CMD_GROUP_SUBDUE_STOP:
			if(ground == GroudAddrGet()){
				SoundEventSet(SOUND_TYPE_SHOCK_STOP);
				EletricPulseSetTime_S(0);
			}
		break;


		case RADIO_PRO_CMD_ALL_SUBDUE_START:
			EletricPulseSetTime_S(ELECTRIC_SHOCK_TIME);
			SoundEventSet(SOUND_TYPE_SHOCK_START);
		break;

		case RADIO_PRO_CMD_TERM_CLOSE_BLOCKING:
			if(dstDev == GetRadioSrcAddr()){
				g_rSysConfigInfo.electricFunc &= 0xffffffff^ELE_FUNC_ENABLE_PREVENT_INSERT;
				SoundEventSet(SOUND_TYPE_INSERT_DETECT_DISABLE);
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_TERM_OPEN_BLOCKING:
			if(dstDev == GetRadioSrcAddr()){
				g_rSysConfigInfo.electricFunc |= ELE_FUNC_ENABLE_PREVENT_INSERT;
				SoundEventSet(SOUND_TYPE_INSERT_DETECT_ENABLE);
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_OPEN_PREVENT_ESCAPE:
			g_rSysConfigInfo.electricFunc |= ELE_FUNC_ENABLE_PREVENT_ESCAPE;
			SoundEventSet(SOUND_TYPE_OPEN_ESCAPE);
			Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
		break;


		case RADIO_PRO_CMD_CLOSE_PREVENT_ESCAPE:
			g_rSysConfigInfo.electricFunc &= 0xffffffff^ELE_FUNC_ENABLE_PREVENT_ESCAPE;
			SoundEventSet(SOUND_TYPE_CLOSE_ESCAPE);
			Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
		break;

		case RADIO_PRO_CMD_OPEN_TERMINAL_PREVENT_ESCAPE:
			if(dstDev == GetRadioSrcAddr()){
				g_rSysConfigInfo.electricFunc |= ELE_FUNC_ENABLE_PREVENT_ESCAPE;
				SoundEventSet(SOUND_TYPE_OPEN_ESCAPE);
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_CLOSE_TERMINAL_PREVENT_ESCAPE:
			if(dstDev == GetRadioSrcAddr()){
				g_rSysConfigInfo.electricFunc &= 0xffffffff^ELE_FUNC_ENABLE_PREVENT_ESCAPE;
				SoundEventSet(SOUND_TYPE_CLOSE_ESCAPE);
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;

		case RADIO_PRO_CMD_OPEN_GROUP_PREVENT_ESCAPE:
			if(ground == GroudAddrGet()){
				g_rSysConfigInfo.electricFunc |= ELE_FUNC_ENABLE_PREVENT_ESCAPE;
				SoundEventSet(SOUND_TYPE_OPEN_ESCAPE);
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;


		case RADIO_PRO_CMD_CLOSE_GROUP_PREVENT_ESCAPE:
			if(ground == GroudAddrGet()){
				g_rSysConfigInfo.electricFunc &= 0xffffffff^ELE_FUNC_ENABLE_PREVENT_ESCAPE;
				SoundEventSet(SOUND_TYPE_CLOSE_ESCAPE);
				Sys_event_post(SYSTEMAPP_EVT_STORE_SYS_CONFIG);
			}
		break;

		case RADIO_PRO_CMD_REQUES_TERM_LOG:
			if(dstDev == GetRadioSrcAddr()){
				if(Flash_get_unupload_items() > 0){
					nodeSendingLog = 1;
					nodegLogCnt = 0;
					RadioCmdSetWithNoRes(RADIO_PRO_CMD_LOG_SEND, srcDev);
				}
			}
		break;



#endif //S_C

#ifdef S_G
		case RADIO_CMD_DESTROY_TYPE:
		insertAlarm(srcDev, ALARM_TYPE_DESTORY);
		break;

		case RADIO_CMD_LOW_VOL_TYPE:
		insertAlarm(srcDev, ALARM_TYPE_LOW_POWER);
		// Menu_low_power_display(srcDev);
		break;

		case RADIO_CMD_INSERT_TYPE:
		insertAlarm(srcDev, ALARM_TYPE_UNWEAR);
		// Menu_not_wearing_well_display(srcDev);
		break;

		case RADIO_PRO_CMD_ALL_RESP:
		Semaphore_post(recAckSemHandle);
		sendRetryTimes = 0;
		RadioCmdClearWithRespon();
		break;
#endif //S_G
	}

#ifdef S_C
	log_opration_record(cmdType,srcDev,ground);
#endif 
}


#ifdef S_C

#define ESCAPE_RSSI		(-55)
//***********************************************************************************
// brief:   analysis the node protocal 
// 
// parameter: 
//***********************************************************************************
void NodeProtocalDispath(EasyLink_RxPacket * protocalRxPacket)
{
	radio_protocal_t	*bufTemp;
    Calendar    calendarTemp;
    uint16_t cmdType;
    uint16_t remaindTimes;
    uint32_t gourndTemp;
    uint32_t dstAddr,srcAddr;

    dstAddr = *((uint32_t*)protocalRxPacket->dstAddr);
    TxFrameRecord_t rxSensorDataAckRecord;
    memset(&rxSensorDataAckRecord, 0, sizeof(TxFrameRecord_t));


    NodeStrategyBuffClear();

	// this buf may be include several message
	bufTemp		= (radio_protocal_t *)protocalRxPacket->payload;

	if(protocalRxPacket->rssi > ESCAPE_RSSI)
		escapeTimeCnt = 0;

    srcAddr = bufTemp->srcAddr;
	cmdType = bufTemp->cmdType;
    gourndTemp = bufTemp->ground;
    remaindTimes = bufTemp->brocastRemainder;

    if(cmdType == RADIO_PRO_CMD_ALL_WAKEUP){
    	calendarTemp.Year       = 2000 + bufTemp->rtc[0];
		calendarTemp.Month      = bufTemp->rtc[1];
		calendarTemp.DayOfMonth = bufTemp->rtc[2];
		calendarTemp.Hours      = bufTemp->rtc[3];
		calendarTemp.Minutes    = bufTemp->rtc[4];
		calendarTemp.Seconds    = bufTemp->rtc[5];
		Rtc_set_calendar(&calendarTemp);
    }


	switch(bufTemp->command)
	{
		case RADIO_PRO_CMD_SINGLE:
		case RADIO_PRO_CMD_GROUND:
			RadioCmdProcess(cmdType, dstAddr, gourndTemp, srcAddr);
			if(dstAddr == GetRadioSrcAddr()){
			// wait for the S_G send the same msg
				Task_sleep((remaindTimes+2)*BROCAST_TIME_MS*CLOCK_UNIT_MS);
				RadioCmdSetWithNoResponBrocast(RADIO_PRO_CMD_ALL_RESP, RADIO_CONTROLER_ADDRESS);
			}
		break;

		
		case RADIO_PRO_CMD_SINGLE_WITH_NO_RESP:
		case RADIO_PRO_CMD_GROUND_WITH_NO_RESP:
			// wait for the S_G send the same msg
			Task_sleep((remaindTimes+2)*BROCAST_TIME_MS*CLOCK_UNIT_MS);
			RadioCmdProcess(cmdType, dstAddr, gourndTemp, srcAddr);
		break;

		default:
		return;

	}
}


#endif // S_C


void RaidoCmdTypePack(uint16_t cmdType)
{
	Calendar calendarTemp;
	uint8_t buff[FLASH_SENSOR_DATA_SIZE];
	uint16_t index = 0;

#ifdef S_G
	// 遥控器请求传输log数据
	if(cmdType == RADIO_PRO_CMD_REQUES_TERM_LOG){
		logReceiveTimeOut = 1;
	}

	// 发送非防逃指令和请求扣子log数据，清零技术，继续发送防逃广播指令
	if(!((cmdType == RADIO_PRO_CMD_ALL_WAKEUP) || (cmdType == RADIO_PRO_CMD_REQUES_TERM_LOG)))
		logReceiveTimeOut = 0;
#endif //S_G
	protocalTxBuf.srcAddr	= GetRadioSrcAddr();
	// protocalTxBuf.len 		= 10+8;

	SetRadioDstAddr(GetRadioDstAddr());

	protocalTxBuf.brocastRemainder = brocastTimes;
	protocalTxBuf.cmdType          = cmdType;
	protocalTxBuf.ground           = GroudAddrGet();

    if(cmdType == RADIO_PRO_CMD_ALL_RESP){
	    protocalTxBuf.vol 	= Battery_get_voltage();
    }
    else if(cmdType == RADIO_PRO_CMD_ALL_WAKEUP){
    	calendarTemp = Rtc_get_calendar();
    	protocalTxBuf.rtc[0] 	= calendarTemp.Year - 2000;
    	protocalTxBuf.rtc[1] 	= calendarTemp.Month;
    	protocalTxBuf.rtc[2] 	= calendarTemp.DayOfMonth;
    	protocalTxBuf.rtc[3] 	= calendarTemp.Hours;
    	protocalTxBuf.rtc[4] 	= calendarTemp.Minutes;
    	protocalTxBuf.rtc[5] 	= calendarTemp.Seconds;
    }

    if(cmdType == RADIO_PRO_CMD_LOG_SEND){
    	index = sprintf((char*)protocalTxBuf.load,"%d:", nodegLogCnt);
    	Flash_load_sensor_data_history(buff, FLASH_SENSOR_DATA_SIZE, nodegLogCnt);
    	memcpy((char*)(protocalTxBuf.load+index), buff, strlen((char*)(buff)));
    	index += strlen((char*)(buff));
	    RadioCopyPacketToBuf(((uint8_t*)&protocalTxBuf), index+13, 0, 0, 0);
    }else{
	    RadioCopyPacketToBuf(((uint8_t*)&protocalTxBuf), 13, 0, 0, 0);
    }
}
//***********************************************************************************
// brief:   send low vol event
// 
// parameter: 
// srcAddr:	the concenter radio addr
// dstAddr:	the node radio addr
//***********************************************************************************
void RadioSendWithResp(uint16_t cmdType)
{

	protocalTxBuf.command	= RADIO_PRO_CMD_SINGLE;
	// protocalTxBuf.dstAddr	= GetRadioDstAddr();
	RaidoCmdTypePack(cmdType);
}



//***********************************************************************************
// brief:   send insert event
// 
// parameter: 
// srcAddr:	the concenter radio addr
// dstAddr:	the node radio addr
//***********************************************************************************
void RadioSendWithNoResp(uint16_t cmdType)
{
	protocalTxBuf.command	= RADIO_PRO_CMD_SINGLE_WITH_NO_RESP;
	// protocalTxBuf.dstAddr	= GetRadioDstAddr();

	RaidoCmdTypePack(cmdType);
}


//***********************************************************************************
// brief:   send low vol event
// 
// parameter: 
// srcAddr:	the concenter radio addr
// dstAddr:	the node radio addr
//***********************************************************************************
void RadioSendGroundWithResp(uint16_t cmdType)
{
	protocalTxBuf.command	= RADIO_PRO_CMD_GROUND;
	// protocalTxBuf.dstAddr	= GetRadioDstAddr();
	RaidoCmdTypePack(cmdType);
}



//***********************************************************************************
// brief:   send insert event
// 
// parameter: 
// srcAddr:	the concenter radio addr
// dstAddr:	the node radio addr
//***********************************************************************************
void RadioSendGroundWithNoResp(uint16_t cmdType)
{
	protocalTxBuf.command	= RADIO_PRO_CMD_GROUND_WITH_NO_RESP;
	// protocalTxBuf.dstAddr	= GetRadioDstAddr();
	RaidoCmdTypePack(cmdType);
}

uint16_t testTermVol;

uint16_t GetTestTermVol(void)
{
    uint16_t batValue = 0;
    batValue = ((testTermVol-BAT_VOLTAGE_LOW )*100)/ (BAT_VOLTAGE_FULL-BAT_VOLTAGE_LOW);
    if(batValue > 99)
        batValue = 99;
    if(batValue < 0)
        batValue = 0;
	return batValue;
}
//***********************************************************************************
// brief:   analysis the concenter protocal
// 
// parameter: 
//***********************************************************************************
void ConcenterProtocalDispath(EasyLink_RxPacket * protocalRxPacket)
{
	radio_protocal_t	*bufTemp;
    uint16_t cmdType;
    //uint16_t remaindTimes;
    uint32_t gourndTemp;
    uint32_t srcAddr;
    //uint32_t dstAdd;
    //dstAddr = *((uint32_t*)protocalRxPacket->dstAddr);


	concenterRemainderCache = EASYLINK_MAX_DATA_LENGTH;
	bufTemp                 = (radio_protocal_t *)protocalRxPacket->payload;

	ClearRadioSendBuf();

	srcAddr = bufTemp->srcAddr;
	cmdType = bufTemp->cmdType;
    gourndTemp = bufTemp->ground;
    //remaindTimes = bufTemp->brocastRemainder;

	SetRadioDstAddr(srcAddr);
	
    testTermVol = bufTemp->vol;

	if(RADIO_PRO_CMD_LOG_SEND == cmdType){
		UsbSend_NodeConfig(EV_Send_Term_Log, bufTemp->load, strlen((char*)(bufTemp->load)));
		return;
	}
	switch(bufTemp->command)
	{
		case RADIO_PRO_CMD_SINGLE:
		case RADIO_PRO_CMD_SINGLE_WITH_NO_RESP:
		case RADIO_PRO_CMD_GROUND:
		case RADIO_PRO_CMD_GROUND_WITH_NO_RESP:
			RadioCmdProcess(cmdType, srcAddr, gourndTemp, srcAddr);
		break;

		default:
	    break;
	}

    Sys_event_post(SYSTEMAPP_EVT_DISP);
}


uint32_t IntToHex(uint32_t iData)
{
	uint32_t hexData = 0;
	uint8_t i;
	for(i = 0; i<8; i++){
		hexData |= ((iData%10) << (4*i));
		iData /= 10;
	}
	return hexData;
}


void GroudAddrSet(uint32_t ground)
{
	groundAddr = ground;
}

uint32_t GroudAddrGet(void)
{
	return groundAddr;
}

//
void RadioCmdSetWithNoRes(uint16_t cmd, uint32_t dstAddr)
{
	cmdType = cmd;
	cmdEvent |= (0x1 << cmd);
	if(dstAddr){
		SetRadioDstAddr(dstAddr);
	}
	RadioSingleSend();
}

bool RadioCmdSetWithNoResponBrocast(uint16_t cmd, uint32_t dstAddr)
{
	if(dstAddr){
		SetRadioDstAddr(dstAddr);
	}
	cmdTypeGroud = cmd;
	cmdEventGroud |= (0x1 << cmd);
	RadioSendBrocast();

	return true;
}


//
void RadioCmdClearWithNoRespon(void)
{
	uint8_t i;
	if(nodeSendingLog == 0)
		cmdEvent &= CMD_EVT_ALL ^ (0x1 << cmdType);
	else{
		nodegLogCnt++;
		if(nodegLogCnt >= Flash_get_unupload_items()){
			cmdEvent &= CMD_EVT_ALL ^ (0x1 << cmdType);
			nodeSendingLog = 0;
		}
	}

	cmdType = 0;
	if(cmdEvent){
		for(i = 0; i < CMD_EVENT_MAX; i++){
			if(cmdEvent & (0x1 << i)){
				cmdType = i;
				break;
			}
		}
		RadioSingleSend();
	}
}

uint32_t RadioWithNoResPack(void)
{
	RadioSendWithNoResp(cmdType);
	return cmdType;
}

// 
bool RadioCmdSetWithNoRespon(uint16_t cmd, uint32_t dstAddr, uint32_t ground)
{
	dstAddr = IntToHex(dstAddr);
	ground  = IntToHex(ground);
	GroudAddrSet(ground);

	if(
	(cmd == RADIO_PRO_CMD_TERM_ADD_TO_GROUP) ||
	(cmd == RADIO_PRO_CMD_TERM_DELETE_FROM_GROUP) ||
	(cmd == RADIO_PRO_CMD_TERM_TEST) ||
	(cmd == RADIO_PRO_CMD_TERM_CLOSE_CTROL) ||
	(cmd == RADIO_PRO_CMD_TERM_OPEN_CTROL) ||
	(cmd == RADIO_PRO_CMD_TERM_UNLOCKING) ||
	(cmd == RADIO_PRO_CMD_TERM_POWER_HIGH) ||
	(cmd == RADIO_PRO_CMD_TERM_POWER_MID) ||
	(cmd == RADIO_PRO_CMD_TERM_POWER_LOW) ||
	(cmd == RADIO_PRO_CMD_FIXED_TERM_SUBDUE_START) ||
	(cmd == RADIO_PRO_CMD_FIXED_TERM_SUBDUE_STOP) ||
	(cmd == RADIO_PRO_CMD_TERM_CLOSE_BLOCKING) ||
	(cmd == RADIO_PRO_CMD_TERM_OPEN_BLOCKING) ||
	(cmd == RADIO_PRO_CMD_OPEN_TERMINAL_PREVENT_ESCAPE) ||
	(cmd == RADIO_PRO_CMD_CLOSE_TERMINAL_PREVENT_ESCAPE) ||
	(cmd == RADIO_PRO_CMD_LOG_SEND) ||
	(cmd == RADIO_PRO_CMD_REQUES_TERM_LOG)){
		SetRadioDstAddr(dstAddr);
	}
	else{
		SetRadioDstAddr(RADIO_BROCAST_ADDRESS);
	}


	cmdTypeGroud  = cmd;
	cmdEventGroud |= (0x1 << cmd);
	RadioSendBrocast();

#ifdef ZKS_S6_6_WOR_G
    log_opration_record(cmd,dstAddr,ground);
#endif

	return true;
}

// 
void RadioCmdClearWithNoRespon_Groud(void)
{
	uint8_t i;

	cmdEventGroud &= CMD_EVT_ALL ^ (0x1 << cmdTypeGroud);
	cmdTypeGroud = 0;
	if(cmdEventGroud){
		for(i = 0; i < CMD_EVENT_MAX; i++){
			if(cmdEventGroud & (0x1 << i)){
				cmdTypeGroud = i;
				break;
			}
		}
		RadioSendBrocast();
	}
}

uint32_t RadioWithNoRes_GroudPack(void)
{
	RadioSendGroundWithNoResp(cmdTypeGroud);
	return cmdTypeGroud;
}



// 鍙戦�佺殑闇�瑕佸洖澶嶅懡浠�
bool RadioCmdSetWithRespon(uint16_t cmd, uint32_t dstAddr, uint32_t ground)
{
	dstAddr = IntToHex(dstAddr);
	ground  = IntToHex(ground);
	GroudAddrSet(ground);
	cmdTypeWithRespon = cmd;
	cmdEventWithRespon |= (0x1 << cmd);
	if(dstAddr){
		SetRadioDstAddr(dstAddr);
	}
	sendRetryTimes = RETRY_TIMES;
#ifdef S_G

	RadioSendBrocast();
	Semaphore_pend(recAckSemHandle, BIOS_NO_WAIT);
	WdtClear();
#ifdef ZKS_S6_6_WOR_G
    log_opration_record(cmd,dstAddr,ground);
#endif
	return Semaphore_pend(recAckSemHandle, 4 * CLOCK_UNIT_S);
	// return true;
#else
	RadioSend();
	return true;
#endif

}


void RadioCmdClearWithRespon(void)
{
	uint8_t i;
	if(sendRetryTimes == 0){
		cmdEventWithRespon &= CMD_EVT_ALL ^ (0x1 << cmdTypeWithRespon);
		cmdTypeWithRespon = 0;
		sendRetryTimes = RETRY_TIMES;

		if(cmdEventWithRespon){
			for(i = 0; i < CMD_EVENT_MAX; i++){
				if(cmdEventWithRespon & (0x1 << i)){
					cmdTypeWithRespon = i;
					break;
				}
			}
#ifdef S_G
			RadioSendBrocast();
#else
			RadioSend();
#endif
		}
	}
	else{
		if(cmdEventWithRespon){
		    sendRetryTimes--;
#ifdef S_G
			RadioSendBrocast();
#else
			RadioSend();
#endif
		}
	}
	
}

uint32_t RadioWithResPack(void)
{
	RadioSendGroundWithResp(cmdTypeWithRespon);
	return cmdTypeWithRespon;
}
