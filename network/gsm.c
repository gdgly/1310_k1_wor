//***********************************************************************************
// Copyright 2017, Zksiot Development Ltd.
// Created by Linjie, 2017.08.08
// MCU:	MSP430F5529
// OS: TI-RTOS
// Project:
// File name: gsm.c
// Description: gsm process routine.
//***********************************************************************************
#include "../general.h"

#ifdef SUPPORT_GSM
#include "gsm.h"


#define     GSM_TIMEOUT_MS          20

#define GSM_POWER_PIN               IOID_2
#define GSM_KEY_PIN                 IOID_3

#define Gsm_power_ctrl(on)          PIN_setOutputValue(gsmPinHandle, GSM_POWER_PIN, on)
#define Gsm_pwrkey_ctrl(on)         PIN_setOutputValue(gsmPinHandle, GSM_KEY_PIN, !(on))



static const PIN_Config gsmPinTable[] = {
    GSM_POWER_PIN | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,       /* LED initially off          */
    GSM_KEY_PIN | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,       /* LED initially off          */
    PIN_TERMINATE
};


static void Gsm_event_post(UInt event);
static void Gsm_init(Nwk_Params *params);
static uint8_t Gsm_open(void);
static uint8_t Gsm_close(void);
static uint8_t Gsm_control(uint8_t cmd, void *arg);
static void Gsm_error_indicate(void);
static void Gsm_hwiIntCallback(void);
static void GsmRxToutCb(UArg arg0);
const Nwk_FxnTable Gsm_FxnTable = {
    Gsm_init,
    Gsm_open,
    Gsm_close,
    Gsm_control,
};

static Event_Struct gsmEvtStruct;
static Event_Handle gsmEvtHandle;
static Swi_Struct gsmRxSwiStruct;
static Swi_Handle gsmRxSwiHandle;

GsmObject_t rGsmObject;

static PIN_State   gsmPinState;
static PIN_Handle  gsmPinHandle;


/* Clock for node period sending */

Clock_Struct gsmTimeOutClock;     /* not static so you can see in ROV */
Clock_Handle gsmTimeOutClockHandle;
uint8_t gsmBusyFlag = 0;
uint8_t gsmTimerFlag = 0;
uint16_t gsmRxLength;

#ifdef SUPPORT_NETWORK_SYC_RTC
Calendar networkCalendar;
#endif //SUPPORT_NETWORK_SYC_RTC
//***********************************************************************************
//
// Gsm module io init.
//
//***********************************************************************************
static void Gsm_io_init(void)
{
    gsmPinHandle = PIN_open(&gsmPinState, gsmPinTable);
}

//***********************************************************************************
//
// Gsm module send AT command.
//
//***********************************************************************************
static void AT_send_data(uint8_t *pData, uint16_t length)
{
    UInt key;

    /* Disable preemption. */
    key = Hwi_disable();
    g_rUart1RxData.length = 0;
    Hwi_restore(key);

    Uart_send_burst_data(UART_0, pData, length);
}

//***********************************************************************************
//
// Gsm module send AT command.
//
//***********************************************************************************
static void AT_send_cmd(uint8_t *string)
{
    UInt key;

    while(gsmBusyFlag)
    {
        Task_sleep(5 * CLOCK_UNIT_MS);
    }
    /* Disable preemption. */
    key = Hwi_disable();
    g_rUart1RxData.length = 0;
    Hwi_restore(key);

    Uart_send_string(UART_0, string);
}

//***********************************************************************************
//
// Send ATCMD_UART_SYNC.
//
//***********************************************************************************
static void AT_save_setting(void)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    AT_send_cmd(ATCMD_SAVE_SETTING);
}

//***********************************************************************************
//
// Send ATCMD_UART_SYNC.
//
//***********************************************************************************
static void AT_uart_sync(void)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    AT_send_cmd(ATCMD_UART_SYNC);
}

//***********************************************************************************
//
// Send ATCMD_UART_SET_BAUDRATE.
//
//***********************************************************************************
static void AT_uart_set_baudrate(void)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    AT_send_cmd(ATCMD_UART_SET_BAUDRATE);
}

//***********************************************************************************
//
// Set echo mode cmd.
//
//***********************************************************************************
static void AT_set_echo_mode(uint8_t on)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    if (on) {
        AT_send_cmd(ATCMD_ECHO_ON);
    } else {
        AT_send_cmd(ATCMD_ECHO_OFF);
    }
}

#if 0
//***********************************************************************************
//
// Set low battery voltage alarm mode cmd.
//
//***********************************************************************************
static void AT_set_bat_alarm_mode(uint8_t on)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    if (on) {
        AT_send_cmd(ATCMD_BAT_ALARM_ON);
    } else {
        AT_send_cmd(ATCMD_BAT_ALARM_OFF);
    }
}

//***********************************************************************************
//
// Set low battery voltage shut down mode cmd.
//
//***********************************************************************************
static void AT_set_bat_shut_mode(uint8_t on)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    if (on) {
        AT_send_cmd(ATCMD_BAT_SHUT_ON);
    } else {
        AT_send_cmd(ATCMD_BAT_SHUT_OFF);
    }
}
#endif

//***********************************************************************************
//
// Send ATCMD_SIM_QUERY.
//
//***********************************************************************************
static void AT_sim_query(void)
{
    rGsmObject.cmdType = AT_CMD_SIM_QUERY;
    AT_send_cmd(ATCMD_SIM_QUERY);
}

//***********************************************************************************
//
// Send ATCMD_SIM_CCID.
//
//***********************************************************************************
static void AT_sim_ccid(void)
{
    rGsmObject.cmdType = AT_CMD_SIM_CCID;
    AT_send_cmd(ATCMD_SIM_CCID);
}

//***********************************************************************************
//
// Send ATCMD_CSQ_QUERY.
//
//***********************************************************************************
static void AT_csq_query(void)
{
    rGsmObject.cmdType = AT_CMD_CSQ_QUERY;
    AT_send_cmd(ATCMD_CSQ_QUERY);
}

//***********************************************************************************
//
// Send ATCMD_CREG_QUERY.
//
//***********************************************************************************
static void AT_creg_query(void)
{
    rGsmObject.cmdType = AT_CMD_CREG_QUERY;
    AT_send_cmd(ATCMD_CREG_QUERY);
}

//***********************************************************************************
//
// Send ATCMD_CGREG_QUERY.
//
//***********************************************************************************
static void AT_cgreg_query(void)
{
    rGsmObject.cmdType = AT_CMD_CGREG_QUERY;
    AT_send_cmd(ATCMD_CGREG_QUERY);
}


//***********************************************************************************
//
// Send AT_set_apn.
//
//***********************************************************************************
static void AT_set_apn(void)
{
    uint8_t buf[sizeof(ATCMD_SET_APN) + sizeof(g_rSysConfigInfo.apnuserpwd)];

    sprintf((char*)buf, ATCMD_SET_APN,g_rSysConfigInfo.apnuserpwd);
    rGsmObject.cmdType = AT_CMD_COMMON;
    AT_send_cmd(buf);
}

//***********************************************************************************
//
// Send ATCMD_START_TASK.
//
//***********************************************************************************
static void AT_start_task(void)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    AT_send_cmd(ATCMD_START_TASK);
}

//***********************************************************************************
//
// Send ATCMD_ACT_MS.
//
//***********************************************************************************
static void AT_active_ms(void)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    AT_send_cmd(ATCMD_ACT_MS);
}

#if 0
//***********************************************************************************
//
// Send ATCMD_GET_LOCAL_IP.
//
//***********************************************************************************
static void AT_get_local_ip(void)
{
    rGsmObject.cmdType = AT_CMD_GET_LOCAL_IP;
    AT_send_cmd(ATCMD_GET_LOCAL_IP);
}
#endif
void AT_set_multil_tcp_link(void)
{
#ifdef SUPPORT_TCP_MULTIL_LINK
    rGsmObject.cmdType = AT_CMD_COMMON;
    AT_send_cmd(ATCMD_ENABLE_MULTIL_LINK);
#else
    rGsmObject.cmdType = AT_CMD_COMMON;
    AT_send_cmd(ATCMD_DISABLE_MULTIL_LINK);
#endif //SUPPORT_TCP_MULTIL_LINK

}

//***********************************************************************************
//
// Send ATCMD_START_CONNECT.
//
//***********************************************************************************
static void AT_set_connect_domain(void)
{
    uint8_t buff[20];
    
    if(strlen((const char *)g_rSysConfigInfo.serverAddr)> 0) {//鍚嶇О浼樺�
        sprintf((char *)buff, ATCMD_SET_DOMAINORIP, 1);
    }
    else{
        sprintf((char *)buff, ATCMD_SET_DOMAINORIP, 0);
    }

    rGsmObject.cmdType = AT_CMD_COMMON;
    
    AT_send_cmd(buff);
}

//***********************************************************************************
//
// Send ATCMD_START_MUL_CONNECT.
//
//***********************************************************************************
void AT_start_multil_connect(uint8_t linkIndex)
{
    uint8_t buff[64+20], index, length;

    index = sprintf((char *)buff, ATCMD_START_MUL_CONNECT, linkIndex);

    if(linkIndex == 1){
        if(strlen((const char *)upgradeSeverIp) > 0){
            length = sprintf((char *)(buff + index), "\"%s\",\"%d\"\r\n", upgradeSeverIp, upgradeSeverPort);
        }
        else if(strlen((const char *)g_rSysConfigInfo.serverAddr)> 0) {//鍚嶇О浼樺�
            length = sprintf((char *)(buff + index), "\"%s\",\"%d\"\r\n", g_rSysConfigInfo.serverAddr, g_rSysConfigInfo.serverIpPort);
        
        }
        else{
            length = sprintf((char *)(buff + index), "\"%d.%d.%d.%d\",\"%d\"\r\n", g_rSysConfigInfo.serverIpAddr[0],
                                g_rSysConfigInfo.serverIpAddr[1], g_rSysConfigInfo.serverIpAddr[2],
                                g_rSysConfigInfo.serverIpAddr[3], g_rSysConfigInfo.serverIpPort);
        }
        index += length;
        buff[index] = '\0';

        rGsmObject.cmdType = AT_CMD_CONNECT;
        AT_send_cmd(buff);
    }
    else if(linkIndex == 0 ){
        length = sprintf((char *)(buff + index), MULTIL_TCP_LINK0_ADDR);
        index += length;
        buff[index] = '\0';

        rGsmObject.cmdType = AT_CMD_CONNECT;
        AT_send_cmd(buff);
    }
}

//***********************************************************************************
//
// Send ATCMD_START_CONNECT.
//
//***********************************************************************************
void AT_start_connect(void)
{
    uint8_t buff[64+20], index, length;

    strcpy((char *)buff, ATCMD_START_CONNECT);
    index = sizeof(ATCMD_START_CONNECT) - 1;
    
    if(strlen((const char *)upgradeSeverIp) > 0){
        length = sprintf((char *)(buff + index), "\"%s\",\"%d\"\r\n", upgradeSeverIp, upgradeSeverPort);
    }
    else if(strlen((const char *)g_rSysConfigInfo.serverAddr)> 0) {//鍚嶇О浼樺�
        length = sprintf((char *)(buff + index), "\"%s\",\"%d\"\r\n", g_rSysConfigInfo.serverAddr, g_rSysConfigInfo.serverIpPort);
    
    }
    else{
        length = sprintf((char *)(buff + index), "\"%d.%d.%d.%d\",\"%d\"\r\n", g_rSysConfigInfo.serverIpAddr[0],
                            g_rSysConfigInfo.serverIpAddr[1], g_rSysConfigInfo.serverIpAddr[2],
                            g_rSysConfigInfo.serverIpAddr[3], g_rSysConfigInfo.serverIpPort);
    }
    index += length;
    buff[index] = '\0';

    rGsmObject.cmdType = AT_CMD_CONNECT;
    AT_send_cmd(buff);
}

//***********************************************************************************
//
// Send ATCMD_START_CONNECT.
//
//***********************************************************************************
#ifdef SUPPORT_TCP_MULTIL_LINK
static void AT_tcp_start_send_data(uint16_t length, uint8_t linkIndex)
#else
static void AT_tcp_start_send_data(uint16_t length)
#endif //SUPPORT_TCP_MULTIL_LINK
{
    uint8_t buff[20], index, len;

    strcpy((char *)buff, ATCMD_SEND_DATA);
    index = sizeof(ATCMD_SEND_DATA) - 1;
#ifdef SUPPORT_TCP_MULTIL_LINK
    len = sprintf((char *)(buff + index), "%d,%d\r", linkIndex, length);
#else
    len = sprintf((char *)(buff + index), "%d\r", length);
#endif //SUPPORT_TCP_MULTIL_LINK
    index += len;
    buff[index] = '\0';
    rGsmObject.cmdType = AT_CMD_START_SEND_DATA;
    AT_send_cmd(buff);
}

//***********************************************************************************
//
// Send ATCMD_START_CONNECT.
//
//***********************************************************************************
static void AT_tcp_send_data(uint8_t *pBuff, uint16_t length)
{
    rGsmObject.cmdType = AT_CMD_SEND_DATA;
    AT_send_data(pBuff, length);
}

//***********************************************************************************
//
// Send ATCMD_ACK_QUERY.
//
//***********************************************************************************
#ifdef SUPPORT_TCP_MULTIL_LINK
static void AT_ack_query(uint8_t linkIndex)
{
    uint8_t buff[20];
    
    sprintf((char *)buff, ATCMD_ACK_QUERY_MULTIL, linkIndex);
    rGsmObject.cmdType = AT_CMD_ACK_QUERY;
    AT_send_cmd(buff);
}

#else
static void AT_ack_query(void)
{
    rGsmObject.cmdType = AT_CMD_ACK_QUERY;
    AT_send_cmd(ATCMD_ACK_QUERY);
}
#endif //SUPPORT_TCP_MULTIL_LINK
//***********************************************************************************
//
// Send ATCMD_CLOSE_CONNECT.
//
//***********************************************************************************
#ifdef SUPPORT_TCP_MULTIL_LINK
static void AT_close_connect(uint8_t linkIndex)
{
    uint8_t buff[20];
    
    sprintf((char *)buff, ATCMD_CLOSE_CONNECT_MULTIL, linkIndex);
    rGsmObject.cmdType = AT_CMD_CLOSE_CONNECT;
    AT_send_cmd(buff);
}

#else
static void AT_close_connect(void)
{
    rGsmObject.cmdType = AT_CMD_CLOSE_CONNECT;
    AT_send_cmd(ATCMD_CLOSE_CONNECT);
}
#endif //SUPPORT_TCP_MULTIL_LINK

//***********************************************************************************
//
// Send ATCMD_DEACT_MS.
//
//***********************************************************************************
static void AT_deactive_ms(void)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    AT_send_cmd(ATCMD_DEACT_MS);
}

//***********************************************************************************
//
// Set sleep mode cmd.
//
//***********************************************************************************
static void AT_set_sleep_mode(uint8_t on)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    if (on) {
        AT_send_cmd(ATCMD_SLEEP_ON);
    } else {
        AT_send_cmd(ATCMD_SLEEP_OFF);
    }
}

//***********************************************************************************
//
// Send ATCMD_SET_RECEIVE_HEAD.
//
//***********************************************************************************
static void AT_set_receive_head(void)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    AT_send_cmd(ATCMD_SET_RECEIVE_HEAD);
}

#ifdef USE_QUECTEL_API_FOR_LBS
//***********************************************************************************
//
// Send ATCMD_GET_LOCATION.
//
//***********************************************************************************
static void AT_lbs_get_location(void)
{
    rGsmObject.cmdType = AT_CMD_GET_LOCATION;
    AT_send_cmd(ATCMD_GET_LOCATION);
}
#endif

//***********************************************************************************
//
// Send ATCMD_GET_LOCATION.
//
//***********************************************************************************
static void AT_auto_answer(void)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    AT_send_cmd(ATCMD_AUTO_ANSWER);
}

#ifdef USE_ENGINEERING_MODE_FOR_LBS
//***********************************************************************************
//
// Set engineering mode cmd.
//
//***********************************************************************************
static void AT_set_eng_mode(uint8_t on)
{
    rGsmObject.cmdType = AT_CMD_COMMON;
    if (on) {
        AT_send_cmd(ATCMD_ENG_MODE_ON);
    } else {
        AT_send_cmd(ATCMD_ENG_MODE_OFF);
    }
}

//***********************************************************************************
//
// Send ATCMD_ENG_MODE_QUERY.
//
//***********************************************************************************
static void AT_eng_mode_query(void)
{
    rGsmObject.cmdType = AT_CMD_ENG_MODE_QUERY;
    AT_send_cmd(ATCMD_ENG_MODE_QUERY);
}
#endif

#ifdef SUPPORT_IMEI
//***********************************************************************************
//
// Send ATCMD_IMEI_QUERY.
//
//***********************************************************************************
static void AT_imei_query(void)
{
    rGsmObject.cmdType = AT_CMD_IMEI_QUERY;
    AT_send_cmd(ATCMD_IMEI_QUERY);
}
#endif

#ifdef SUPPORT_NETWORK_SYC_RTC
//***********************************************************************************
//
// Send ATCMD_ENABLE_SYC_TIME.
//
//***********************************************************************************
static void AT_syc_rtc_enable(void)
{
    rGsmObject.cmdType = AT_CMD_ENABLE_SYC_RTC;
    AT_send_cmd(ATCMD_ENABLE_SYC_TIME);
}

//***********************************************************************************
//
// Send ATCMD_DISABLE_SYC_RTC.
//
//***********************************************************************************
static void AT_syc_rtc_disable(void)
{
    rGsmObject.cmdType = AT_CMD_DISABLE_SYC_RTC;
    AT_send_cmd(ATCMD_DISABLE_SYC_TIME);
}
static UInt Gsm_wait_ack(uint32_t timeout);
//***********************************************************************************
//
// Send ATCMD_RTC_QUERY.
//
//***********************************************************************************
static void AT_rtc_query(void)
{
    rGsmObject.cmdType = AT_CMD_RTC_QUERY;
    AT_send_cmd(ATCMD_RTC_QUERY);
}

static GSM_RESULT Gsm_enable_syn_rtc(void)
{
    UInt eventId;
    GSM_RESULT result = RESULT_ERROR;

    AT_syc_rtc_enable();
    eventId = Gsm_wait_ack(6000);
    if(eventId & GSM_EVT_CMD_OK)
        result = RESULT_OK;
    
    return result;
}


GSM_RESULT Gsm_get_rtc(void)
{
    UInt eventId;
    GSM_RESULT result = RESULT_ERROR;

    AT_syc_rtc_enable();
    eventId = Gsm_wait_ack(6000);
    Task_sleep(2000 * CLOCK_UNIT_MS);
    AT_rtc_query();
    eventId = Gsm_wait_ack(10000);
    if(eventId & GSM_EVT_CMD_OK)
        result = RESULT_OK;
    // AT_syc_rtc_disable();
    // Gsm_wait_ack(6000);
    return result;
}
#endif // SUPPORT_NETWORK_SYC_RTC

//***********************************************************************************
//
// Gsm module wait cmd ack.
//
//***********************************************************************************
static UInt Gsm_wait_ack(uint32_t timeout)
{
    UInt eventId;
    UInt key;

    eventId = Event_pend(gsmEvtHandle, 0, GSM_EVT_SHUTDOWN | GSM_EVT_CMD_OK | GSM_EVT_CMD_ERROR | GSM_EVT_CMD_RECONNECT, timeout * CLOCK_UNIT_MS);

    while(gsmBusyFlag)
    {
        Task_sleep(5 * CLOCK_UNIT_MS);
    }

    rGsmObject.cmdType = AT_CMD_NULL;

    key = Hwi_disable();
    g_rUart1RxData.length = 0;
    Hwi_restore(key);

    return eventId;
}

//***********************************************************************************
//
// Gsm module poweron.
//
//***********************************************************************************
static void Gsm_poweron(void)
{
    if (rGsmObject.state == GSM_STATE_POWEROFF) {
        UartHwInit(UART_0, 38400, Gsm_hwiIntCallback, UART_GSM);

        Gsm_power_ctrl(1);
        Gsm_pwrkey_ctrl(1);
        Task_sleep(200 * CLOCK_UNIT_MS);
        Gsm_pwrkey_ctrl(0);
        Task_sleep(1200 * CLOCK_UNIT_MS);
        Gsm_pwrkey_ctrl(1);
//      Task_sleep(1000 * CLOCK_UNIT_MS);

        rGsmObject.sleep = GSM_SLEEP_OFF;
        rGsmObject.actPDPCnt = 0;
        rGsmObject.uploadFailCnt = 0;
        rGsmObject.state = GSM_STATE_CONFIG;

        Event_pend(gsmEvtHandle, 0, GSM_EVT_ALL, BIOS_NO_WAIT);
    }
}

//***********************************************************************************
//
// Gsm module poweroff.
//
//***********************************************************************************
static void Gsm_poweroff(void)
{
    if (rGsmObject.state != GSM_STATE_POWEROFF) {
        UartClose(UART_0);
        UartPortDisable(UART_GSM);
        Gsm_pwrkey_ctrl(0);
        Task_sleep(1000 * CLOCK_UNIT_MS);
        Gsm_pwrkey_ctrl(1);
        Task_sleep(12000 * CLOCK_UNIT_MS);
        Gsm_power_ctrl(0);
        Task_sleep(800 * CLOCK_UNIT_MS);

        rGsmObject.sleep = GSM_SLEEP_OFF;
        rGsmObject.actPDPCnt = 0;
        rGsmObject.uploadFailCnt = 0;
        rGsmObject.state = GSM_STATE_POWEROFF;
    }
}

//***********************************************************************************
//
// Gsm module reset process.
//
//***********************************************************************************
static void Gsm_reset(void)
{
    UInt eventId = 0;

    Led_ctrl(LED_B, 0, 0, 0);
    Task_sleep(500 * CLOCK_UNIT_MS);
    Gsm_error_indicate();

    Gsm_poweroff();

    if (rGsmObject.resetCnt < 250)
        rGsmObject.resetCnt++;

    if (rGsmObject.resetCnt >= 3) {
        eventId = Event_pend(gsmEvtHandle, 0, GSM_EVT_SHUTDOWN, 10 * 60 * CLOCK_UNIT_S);//淇敼GSM澶嶄綅鏃堕棿涓�10min鍜�5min
    } else if (rGsmObject.resetCnt >= 2) {
        eventId = Event_pend(gsmEvtHandle, 0, GSM_EVT_SHUTDOWN, 5 * 60 * CLOCK_UNIT_S);
    }

    if (!(eventId & GSM_EVT_SHUTDOWN)) {
        Gsm_poweron();
    }
}

//***********************************************************************************
//
// Gsm module config process.
// Return:  RESULT_OK -- config ok
//          RESULT_RESET -- need reset module
//          RESULT_SHUTDOWN -- need shutdown module
//
//***********************************************************************************
static GSM_RESULT Gsm_module_config(void)
{
    uint8_t i;
    UInt eventId;

    rGsmObject.error = GSM_NO_ERR;

    //uart sync
    for (i = 30; i > 0; i--) {
        AT_uart_sync();
        eventId = Gsm_wait_ack(1000);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_UART_SYNC;
        return RESULT_RESET;
    }

    //uart set baudrate
    AT_uart_set_baudrate();
    eventId = Gsm_wait_ack(500);
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    }

    //uart shut echo mode
    AT_set_echo_mode(0);
    eventId = Gsm_wait_ack(500);
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    }

    //uart save setting
    AT_save_setting();
    eventId = Gsm_wait_ack(500);
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    }

    //uart set receive head
    for (i = 5; i > 0; i--) {
        AT_set_receive_head();
        eventId = Gsm_wait_ack(500);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_UART_SYNC;
        return RESULT_RESET;
    }

#ifdef USE_ENGINEERING_MODE_FOR_LBS
    //uart Open engineering mode for lbs
    for (i = 5; i > 0; i--) {
        AT_set_eng_mode(1);
        eventId = Gsm_wait_ack(500);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    }
#endif

#ifdef SUPPORT_IMEI
    //get IMEI
    AT_imei_query();
    eventId = Gsm_wait_ack(500);
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    }
#endif

    return RESULT_OK;
}

//***********************************************************************************
//
// Gsm module gprs network query process.
// Return:  RESULT_OK -- gprs network query ok
//          RESULT_RESET -- need reset module
//          RESULT_SHUTDOWN -- need shutdown module
//
//***********************************************************************************
static GSM_RESULT Gsm_gprs_query(void)
{
    uint8_t i;
    UInt eventId;

    rGsmObject.error = GSM_NO_ERR;

#ifdef SUPPORT_TCP_MULTIL_LINK
    for (i = 30; i > 0; i--) {
        AT_set_multil_tcp_link();
        eventId = Gsm_wait_ack(1000);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_SIM_QUERY;
        return RESULT_RESET;
    }
#endif //SUPPORT_TCP_MULTIL_LINK

    //sim card query
    for (i = 30; i > 0; i--) {
        AT_sim_query();
        eventId = Gsm_wait_ack(1000);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_SIM_QUERY;
        return RESULT_RESET;
    }

    //sim card get ccid
    for (i = 5; i > 0; i--) {
        AT_sim_ccid();
        eventId = Gsm_wait_ack(1000);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_SIM_QUERY;
        return RESULT_RESET;
    }

    //csq query
    for (i = 60; i > 0; i--) {
        AT_csq_query();
        eventId = Gsm_wait_ack(1000);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_CSQ_QUERY;
        return RESULT_RESET;
    }

    //cgreg query
    for (i = 60; i > 0; i--) {
        AT_cgreg_query();
        eventId = Gsm_wait_ack(1000);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_CGREG_QUERY;
        return RESULT_RESET;
    }

    return RESULT_OK;
}

//***********************************************************************************
//
// Gsm module active PDP process.
// Return:  RESULT_OK -- active PDP ok
//          RESULT_RESET -- need reset module
//          RESULT_SHUTDOWN -- need shutdown module
//          RESULT_GPRS_QUERY -- need goto gprs query
//
//***********************************************************************************
static GSM_RESULT Gsm_active_PDP(void)
{
    UInt eventId;

    rGsmObject.error = GSM_NO_ERR;

    //璁剧疆鎺ュ叆鐐� APN銆佺敤鎴峰悕鍜屽瘑鐮�
    if (g_rSysConfigInfo.apnuserpwd[0]){
        AT_set_apn();
        eventId = Gsm_wait_ack(500);
        if (eventId & GSM_EVT_SHUTDOWN){
            return RESULT_SHUTDOWN;
        }
    }
    
    //鍚姩浠诲姟
    AT_start_task();
    eventId = Gsm_wait_ack(500);
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    }

    //婵�娲荤Щ鍔ㄥ�鏅紙鎴栧彂璧� GPRS/CSD 鏃犵嚎杩炴帴锛�
    AT_active_ms();
    eventId = Gsm_wait_ack(150000);
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (eventId == 0) {
        //timeout
        rGsmObject.error = GSM_ERR_ACT;
        return RESULT_RESET;
    } else if (eventId & GSM_EVT_CMD_ERROR) {
        //鍘绘縺娲诲満鏅�
        AT_deactive_ms();
        eventId = Gsm_wait_ack(90000);
        if (eventId & GSM_EVT_SHUTDOWN) {
            return RESULT_SHUTDOWN;
        } else if (eventId & GSM_EVT_CMD_OK) {
            return RESULT_GPRS_QUERY;
        } else {
            //timeout or de-act fail
            rGsmObject.error = GSM_ERR_DEACT;
            return RESULT_RESET;
        }
    }

    return RESULT_OK;
}

//***********************************************************************************
//
// Gsm module tcp connect process.
// Return:  RESULT_OK -- tcp connect ok
//          RESULT_RESET -- need reset module
//          RESULT_SHUTDOWN -- need shutdown module
//          RESULT_GPRS_QUERY -- need goto gprs query
//
//***********************************************************************************
static GSM_RESULT Gsm_tcp_connect(uint8_t linkIndex)
{
    uint8_t i;
    UInt eventId;

    rGsmObject.error = GSM_NO_ERR;

    //寤虹� TCP 杩炴帴鎴栨敞鍐� UDP 绔彛鍙�
    for (i = 5; i > 0; i--) {
        AT_set_connect_domain();
        eventId = Gsm_wait_ack(500);
        if (!(eventId & GSM_EVT_CMD_OK))
            continue;            
#ifdef SUPPORT_TCP_MULTIL_LINK
        AT_start_multil_connect(linkIndex);
#else
        AT_start_connect();
#endif //SUPPORT_TCP_MULTIL_LINK
        eventId = Gsm_wait_ack(90000);
        if (!(eventId & GSM_EVT_CMD_ERROR))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0 || eventId == 0) {
        //鍘绘縺娲诲満鏅�
        AT_deactive_ms();
        eventId = Gsm_wait_ack(90000);
        if (eventId & GSM_EVT_SHUTDOWN) {
            return RESULT_SHUTDOWN;
        } else if (eventId & GSM_EVT_CMD_OK) {
            return RESULT_GPRS_QUERY;
        } else {
            //timeout or de-act fail
            rGsmObject.error = GSM_ERR_DEACT;
            return RESULT_RESET;
        }
    }

    return RESULT_OK;
}

//***********************************************************************************
//
// Gsm module tcp upload process.
// Return:  RESULT_OK -- tcp upload ok
//          RESULT_RESET -- need reset module
//          RESULT_SHUTDOWN -- need shutdown module
//          RESULT_GPRS_QUERY -- need goto gprs query
//          RESULT_CONNECT -- need goto tcp connect
//
//***********************************************************************************
#ifdef SUPPORT_TCP_MULTIL_LINK
static GSM_RESULT Gsm_tcp_upload(uint8_t *pBuff, uint16_t length, uint8_t linkIndex)
#else
static GSM_RESULT Gsm_tcp_upload(uint8_t *pBuff, uint16_t length)
#endif //SUPPORT_TCP_MULTIL_LINK
{
    uint16_t i;
    UInt eventId;

    rGsmObject.error = GSM_NO_ERR;

    //Get csq value
    AT_csq_query();
    eventId = Gsm_wait_ack(500);
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (!(eventId & GSM_EVT_CMD_OK)) {
        //timeout or error
        rGsmObject.error = GSM_ERR_CSQ_QUERY;
        return RESULT_RESET;
    }

    //tcp send data
    Event_pend(gsmEvtHandle, 0, GSM_EVT_ALL, BIOS_NO_WAIT);
#ifdef SUPPORT_TCP_MULTIL_LINK
    AT_tcp_start_send_data(length, linkIndex);
#else
    AT_tcp_start_send_data(length);
#endif //SUPPORT_TCP_MULTIL_LINK
    eventId = Gsm_wait_ack(500);
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (!(eventId & GSM_EVT_CMD_OK)) {
        //timeout or error
        goto LAB_UPLOAD_CLOSE_CONNECT;
    }
    AT_tcp_send_data(pBuff, length);
    eventId = Gsm_wait_ack(500);
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (!(eventId & GSM_EVT_CMD_OK)) {
        //timeout or error
        goto LAB_UPLOAD_CLOSE_CONNECT;
    }

    //tcp send data ack query
    for (i = 300; i > 0; i--) {
#ifdef SUPPORT_TCP_MULTIL_LINK
        AT_ack_query(linkIndex);
#else
        AT_ack_query();
#endif //SUPPORT_TCP_MULTIL_LINK
        eventId = Gsm_wait_ack(30);
        Task_sleep(270 * CLOCK_UNIT_MS);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN | GSM_EVT_CMD_RECONNECT))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if ((i == 0) || (eventId & GSM_EVT_CMD_RECONNECT)) {
        //timeout
        goto LAB_UPLOAD_CLOSE_CONNECT;
    }

    return RESULT_OK;

LAB_UPLOAD_CLOSE_CONNECT:
#ifdef SUPPORT_TCP_MULTIL_LINK
    AT_close_connect(linkIndex);
    rGsmObject.linkState &= ~(0x01 << linkIndex);
#else
    AT_close_connect();
#endif //SUPPORT_TCP_MULTIL_LINK
    eventId = Gsm_wait_ack(60000);
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (eventId & GSM_EVT_CMD_ERROR) {
        return RESULT_GPRS_QUERY;
    } else if (eventId == 0) {
        //timeout
        rGsmObject.error = GSM_ERR_TCP_CLOSE;
        return RESULT_RESET;
    }

    return RESULT_CONNECT;
}

//***********************************************************************************
//
// Gsm module query csq value process.
// Return:  RESULT_OK -- query csq ok
//          RESULT_ERROR -- query csq error
//          RESULT_SHUTDOWN -- need shutdown module
//
//***********************************************************************************
static GSM_RESULT Gsm_query_csq(void)
{
    UInt eventId;
    uint8_t i ;
    rGsmObject.error = GSM_NO_ERR;

    //Get csq value
    for (i = 5; i > 0; i--) {
        AT_csq_query();
        eventId = Gsm_wait_ack(500);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (!(eventId & GSM_EVT_CMD_OK)) {
        //timeout or error
         rGsmObject.error = GSM_ERR_CSQ_QUERY;
         return RESULT_RESET;
    }

    return RESULT_OK;
}

#ifdef SUPPORT_LBS
//***********************************************************************************
//
// Gsm module get lbs information process.
// Return:  RESULT_OK -- get lbs information ok
//          RESULT_ERROR -- get lbs information error
//          RESULT_SHUTDOWN -- need shutdown module
//
//***********************************************************************************
static GSM_RESULT Gsm_get_lbs(void)
{
    UInt eventId;
    uint8_t i;

    rGsmObject.error = GSM_NO_ERR;

#ifdef USE_QUECTEL_API_FOR_LBS
    AT_lbs_get_location();
    eventId = Gsm_wait_ack(60000);
#elif defined(USE_ENGINEERING_MODE_FOR_LBS)
    for (i = 3; i > 0; i--) {
        AT_eng_mode_query();
        eventId = Gsm_wait_ack(4000);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
#endif
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (eventId == 0 ||(eventId & GSM_EVT_CMD_ERROR)) {
        //timeout
        rGsmObject.error = GSM_ERR_GET_LBS;
        return RESULT_ERROR;
    }

    return RESULT_OK;
}
#endif

//***********************************************************************************
//
// Gsm module sleep process.
// Return:  RESULT_OK -- sleep ok
//          RESULT_RESET -- need reset module
//          RESULT_SHUTDOWN -- need shutdown module
//
//***********************************************************************************
static GSM_RESULT Gsm_sleep(void)
{
    uint8_t i;
    UInt eventId;

    rGsmObject.error = GSM_NO_ERR;

    //set sleep mode
    for (i = 5; i > 0; i--) {
        AT_set_sleep_mode(1);
        eventId = Gsm_wait_ack(500);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_SLEEP_MODE;
        return RESULT_RESET;
    }

    return RESULT_OK;
}

//***********************************************************************************
//
// Gsm module wakeup process.
// Return:  RESULT_OK -- wakeup ok
//          RESULT_RESET -- need reset module
//          RESULT_SHUTDOWN -- need shutdown module
//
//***********************************************************************************
static GSM_RESULT Gsm_wakeup(void)
{
    uint8_t i;
    UInt eventId;

    rGsmObject.error = GSM_NO_ERR;

    AT_uart_sync();
    Task_sleep(200 * CLOCK_UNIT_MS);

    //set sleep mode
    for (i = 5; i > 0; i--) {
        AT_set_sleep_mode(0);
        eventId = Gsm_wait_ack(500);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }

    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_WAKEUP_MODE;
        return RESULT_RESET;
    }

    return RESULT_OK;
}

//***********************************************************************************
//
// Gsm module set auto answer state to test antenna.
// Return:  GSM_NO_ERR -- ok
//
//***********************************************************************************
static GSM_RESULT Gsm_test(void)
{
    uint8_t i;
    UInt eventId;

    rGsmObject.error = GSM_NO_ERR;

    //uart sync
    for (i = 30; i > 0; i--) {
        AT_uart_sync();
        eventId = Gsm_wait_ack(1000);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_UART_SYNC;
        return RESULT_RESET;
    }

    //sim card query
    for (i = 30; i > 0; i--) {
        AT_sim_query();
        eventId = Gsm_wait_ack(1000);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_SIM_QUERY;
        return RESULT_RESET;
    }

    //csq query
    for (i = 60; i > 0; i--) {
        AT_csq_query();
        eventId = Gsm_wait_ack(1000);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_CSQ_QUERY;
        return RESULT_RESET;
    }

    //creg query
    for (i = 60; i > 0; i--) {
        AT_creg_query();
        eventId = Gsm_wait_ack(1000);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_CREG_QUERY;
        return RESULT_RESET;
    }

    //auto answer
    for (i = 200; i > 0; i--) {
        AT_auto_answer();
        eventId = Gsm_wait_ack(500);
        if (eventId & (GSM_EVT_CMD_OK | GSM_EVT_SHUTDOWN))
            break;
    }
    if (eventId & GSM_EVT_SHUTDOWN) {
        return RESULT_SHUTDOWN;
    } else if (i == 0) {
        rGsmObject.error = GSM_ERR_AUTO_ANSWER;
        return RESULT_RESET;
    }

    return RESULT_OK;
}

//***********************************************************************************
//
// Gsm module transmit process.
//
//***********************************************************************************
static GSM_RESULT Gsm_transmit_process(uint8_t *pBuff, uint16_t length, uint8_t linkIndex)
{
    GSM_RESULT result;

//Gsm config
    if (rGsmObject.state == GSM_STATE_CONFIG) {
        result = Gsm_module_config();
        if (result == RESULT_SHUTDOWN || result == RESULT_RESET) {
            return result;
        }
        rGsmObject.state = GSM_STATE_GPRS_QUERY;
    }

LAB_GPRS_QUERY:
//Gsm sim card and gprs network query
    if (rGsmObject.state == GSM_STATE_GPRS_QUERY) {
        result = Gsm_gprs_query();
        if (result == RESULT_SHUTDOWN || result == RESULT_RESET) {
            return result;
        }
        rGsmObject.state = GSM_STATE_ACTIVE_PDP;
    }

//Gsm active PDP
    if (rGsmObject.state == GSM_STATE_ACTIVE_PDP) {
        if (rGsmObject.actPDPCnt++ >= 3) {
            rGsmObject.error = GSM_ERR_ACT;
            return RESULT_RESET;
        }

        result = Gsm_active_PDP();
        if (result == RESULT_SHUTDOWN || result == RESULT_RESET) {
            return result;
        } else if (result == RESULT_GPRS_QUERY) {
            //Active PDP fail
            rGsmObject.state = GSM_STATE_GPRS_QUERY;
            goto LAB_GPRS_QUERY;
        }
        rGsmObject.state = GSM_STATE_TCP_CONNECT;
        rGsmObject.actPDPCnt = 0;
#ifdef SUPPORT_UPLOADTIME_LIMIT
        rGsmObject.linkState = 0;
#endif //SUPPORT_UPLOADTIME_LIMIT
    }

LAB_CONNECT:
//Gsm connect server
#ifdef SUPPORT_TCP_MULTIL_LINK
    if (rGsmObject.state == GSM_STATE_TCP_CONNECT || (!(rGsmObject.linkState & (0x01 << linkIndex)))) {
        result = Gsm_tcp_connect(linkIndex);
#else
    if (rGsmObject.state == GSM_STATE_TCP_CONNECT) {
        result = Gsm_tcp_connect(NULL);
#endif
        if (result == RESULT_SHUTDOWN || result == RESULT_RESET) {
            return result;
        } else if (result == RESULT_GPRS_QUERY) {
            //Active PDP fail
            rGsmObject.state = GSM_STATE_GPRS_QUERY;
#ifdef SUPPORT_TCP_MULTIL_LINK
            rGsmObject.linkState = 0;
#endif  //SUPPORT_TCP_MULTIL_LINK
            goto LAB_GPRS_QUERY;
        }
        rGsmObject.state = GSM_STATE_TCP_UPLOAD;
#ifdef SUPPORT_TCP_MULTIL_LINK
            rGsmObject.linkState |= 0x01 << linkIndex;
#endif  //SUPPORT_TCP_MULTIL_LINK
    }

    if (pBuff != NULL && length != 0) {
        if (rGsmObject.uploadFailCnt++ >= 3) {
            rGsmObject.error = GSM_ERR_UPLOAD;
            return RESULT_RESET;
        }
#ifdef SUPPORT_TCP_MULTIL_LINK
        result = Gsm_tcp_upload(pBuff, length, linkIndex);
#else
        result = Gsm_tcp_upload(pBuff, length);
#endif  //SUPPORT_TCP_MULTIL_LINK
        if (result == RESULT_SHUTDOWN || result == RESULT_RESET) {
            return result;
        } else if (result == RESULT_GPRS_QUERY) {
            rGsmObject.state = GSM_STATE_GPRS_QUERY;
#ifdef SUPPORT_TCP_MULTIL_LINK
            rGsmObject.linkState = 0;
#endif  //SUPPORT_TCP_MULTIL_LINK
            goto LAB_GPRS_QUERY;
        } else if (result == RESULT_CONNECT) {
            //Active PDP fail

            rGsmObject.state = GSM_STATE_TCP_CONNECT;
#ifdef SUPPORT_TCP_MULTIL_LINK
            rGsmObject.linkState &= ~(0x01 << linkIndex);
#endif  //SUPPORT_TCP_MULTIL_LINK
            goto LAB_CONNECT;
        }

        rGsmObject.uploadFailCnt = 0;
        rGsmObject.resetCnt = 0;
    }

    return RESULT_OK;
}

extern void Nwk_data_proc_callback_JSLL(uint8_t *pBuff, uint16_t length);
//***********************************************************************************
//
// Gsm AT command respond monitor.
//
//***********************************************************************************
static void Gsm_rxSwiFxn(void)
{
    char *ptr, *ptr2;
    UInt key;
    uint8_t index;
    uint16_t value, rxLen;
    uint32_t sentNum, ackedNum;
    UartRxData_t tempRx;
#ifdef USE_QUECTEL_API_FOR_LBS
    float latitudetmp;
#endif
#ifdef USE_ENGINEERING_MODE_FOR_LBS
    char *ptr1;
    uint8_t i;
#endif
#ifdef SUPPORT_TCP_MULTIL_LINK
    uint8_t linkIndex;
#endif //SUPPORT_TCP_MULTIL_LINK


    /* Disable preemption. */
    key = Hwi_disable();
    tempRx.length = gsmRxLength;
    memcpy((char *)tempRx.buff, (char *)g_rUart1RxData.buff, tempRx.length);
    tempRx.buff[tempRx.length] = '\0';
    ptr = strstr((char *)tempRx.buff, "IPD");
    if(ptr == NULL)
    {
        if(g_rUart1RxData.length > tempRx.length)
        {
            memcpy((char*)g_rUart1RxData.buff,  (char*)g_rUart1RxData.buff+gsmRxLength, g_rUart1RxData.length - tempRx.length);
        }
        g_rUart1RxData.length = g_rUart1RxData.length - tempRx.length;
    }
    Hwi_restore(key);
    value = tempRx.length;
    ptr2  = (char *)tempRx.buff;
    if (ptr != NULL) {
redispath:
#ifdef SUPPORT_TCP_MULTIL_LINK
        linkIndex = atoi(ptr-6);
#endif //SUPPORT_TCP_MULTIL_LINK
        index = ptr - ptr2;
        rxLen = atoi(ptr + 3);
        if ((value - index) > rxLen && rxLen < UART_BUFF_SIZE) {
            //Second 0x7e
            g_rUart1RxData.length = index;
#ifdef SUPPORT_TCP_MULTIL_LINK
            ptr = strstr((char *)ptr, ":");
            if(linkIndex == 0){
                rGsmObject.dataProcCallbackFxn((uint8_t *)ptr+1, rxLen);
            }
            else if(linkIndex == 1){
                Nwk_data_proc_callback_JSLL((uint8_t *)ptr+1, rxLen);
            }
#else
            ptr = strstr((char *)ptr, "\x7e");
            rGsmObject.dataProcCallbackFxn((uint8_t *)ptr, rxLen);
#endif //SUPPORT_TCP_MULTIL_LINK
            value -= rxLen;
            if(value > 3)
            {
                ptr2 = ptr + rxLen;
                ptr = strstr((char *)(ptr2), "IPD");
                if(ptr != NULL)
                {
                    goto redispath;
                }

            }
        } else {
            //First 0x7e
        }
        return;
    }

#ifdef SUPPORT_NETWORK_SYC_RTC
    ptr =  strstr((char *)tempRx.buff, "+QNITZ:");
    if (ptr != NULL)
    {
        ptr2 = ptr + 9;
        networkCalendar.Year       = atoi(ptr2) + CALENDAR_BASE_YEAR;
        ptr =  strstr(ptr2, "/");
        if (ptr == NULL)
            goto CmdProcess;
        ptr2 = ptr + 1;
        networkCalendar.Month      = atoi(ptr2);
        ptr =  strstr(ptr2, "/");
        if (ptr == NULL)
            goto CmdProcess;
        ptr2 = ptr + 1;
        networkCalendar.DayOfMonth = atoi(ptr2);
        ptr =  strstr(ptr2, ",");
        if (ptr == NULL)
            goto CmdProcess;
        ptr2 = ptr + 1;
        networkCalendar.Hours      = atoi(ptr2);
        ptr =  strstr(ptr2, ":");
        if (ptr == NULL)
            goto CmdProcess;
        ptr2 = ptr + 1;
        networkCalendar.Minutes    = atoi(ptr2);
        ptr =  strstr(ptr2, ":");
        if (ptr == NULL)
            goto CmdProcess;
        ptr2 = ptr + 1;
        networkCalendar.Seconds    = atoi(ptr2);
        index = atoi(ptr2+2);
        networkCalendar.Hours += index/4;   // if the timezone less 0, it will wrong
        networkCalendar.Minutes += index%4;
        Rtc_set_calendar(&networkCalendar);
        Nwk_Ntp_Set();
    }
CmdProcess:
#endif //SUPPORT_NETWORK_SYC_RTC

    switch (rGsmObject.cmdType) {
        case AT_CMD_COMMON:
            ptr = strstr((char *)tempRx.buff, "OK\r\n");
            if (ptr != NULL) {
                Gsm_event_post(GSM_EVT_CMD_OK);
                break;
            }
            ptr = strstr((char *)tempRx.buff, "ERROR");
            if (ptr != NULL) {
                Gsm_event_post(GSM_EVT_CMD_ERROR);
            }
            break;

        case AT_CMD_SIM_QUERY:
            ptr = strstr((char *)tempRx.buff, "OK\r\n");
            if (ptr != NULL) {
                ptr = strstr((char *)tempRx.buff, "READY");
                if (ptr != NULL) {
                    Gsm_event_post(GSM_EVT_CMD_OK);
                }
            }
            break;

        case AT_CMD_SIM_CCID:
            ptr = strstr((char *)tempRx.buff, "OK\r\n");
            if (ptr != NULL) {
                ptr = strstr((char *)tempRx.buff, "+CCID:");
                if (ptr != NULL) {
                    memcpy((char *)rGsmObject.simCcid, ptr + 8, 20);
                    Gsm_event_post(GSM_EVT_CMD_OK);
                }
            }
            break;

        case AT_CMD_CSQ_QUERY:
            ptr = strstr((char *)tempRx.buff, "OK\r\n");
            if (ptr != NULL) {
                ptr = strstr((char *)tempRx.buff, "+CSQ:");
                if (ptr != NULL) {
                    rGsmObject.rssi = (uint8_t)atoi(ptr + 5);
                    if (rGsmObject.rssi != 99) {
                        if(rGsmObject.rssi > 31)rGsmObject.rssi = 31;//澶勭悊鍑虹幇澶т簬31鐨勫紓甯�
                        Gsm_event_post(GSM_EVT_CMD_OK);
                    }
                }
            }
            break;

        case AT_CMD_CREG_QUERY:
            ptr = strstr((char *)tempRx.buff, "OK\r\n");
            if (ptr != NULL) {
                ptr = strstr((char *)tempRx.buff, "+CREG:");
                if (ptr != NULL) {
                    if (*(ptr + 9) == '1' || *(ptr + 9) == '5') {
                        Gsm_event_post(GSM_EVT_CMD_OK);
                    }
                }
            }
            break;

        case AT_CMD_CGREG_QUERY:
            ptr = strstr((char *)tempRx.buff, "OK\r\n");
            if (ptr != NULL) {
                ptr = strstr((char *)tempRx.buff, "+CGREG:");
                if (ptr != NULL) {
                    if (*(ptr + 10) == '1' || *(ptr + 10) == '5') {
                        Gsm_event_post(GSM_EVT_CMD_OK);
                    }
                }
            }
            break;

        case AT_CMD_GET_LOCAL_IP:
            ptr = strstr((char *)tempRx.buff, ".");
            if (ptr != NULL) {
                Gsm_event_post(GSM_EVT_CMD_OK);
            }
            break;

        case AT_CMD_CONNECT:
            ptr = strstr((char *)tempRx.buff, "ERROR");
            if (ptr != NULL) {
                ptr = strstr((char *)tempRx.buff, "ALREADY CONNECT");
                if (ptr != NULL) {
                    Gsm_event_post(GSM_EVT_CMD_OK);
                    break;
                } else {
                    Gsm_event_post(GSM_EVT_CMD_ERROR);
                    break;
                }
            }
            ptr = strstr((char *)tempRx.buff, "CONNECT OK");
            if (ptr != NULL) {
                Gsm_event_post(GSM_EVT_CMD_OK);
                break;
            }
            ptr = strstr((char *)tempRx.buff, "CONNECT FAIL");
            if (ptr != NULL) {
                Gsm_event_post(GSM_EVT_CMD_ERROR);
            }
            break;

        case AT_CMD_START_SEND_DATA:
            ptr = strstr((char *)tempRx.buff, ">");
            if (ptr != NULL) {
                Gsm_event_post(GSM_EVT_CMD_OK);
            }
            ptr = strstr((char *)tempRx.buff, "ERROR");
            if (ptr != NULL) {
                Gsm_event_post(GSM_EVT_CMD_ERROR);
                break;
            }
            break;

        case AT_CMD_SEND_DATA:
            ptr = strstr((char *)tempRx.buff, "SEND OK");
            if (ptr != NULL) {
                Gsm_event_post(GSM_EVT_CMD_OK);
                break;
            }
            ptr = strstr((char *)tempRx.buff, "SEND FAIL");
            if (ptr != NULL) {
                Gsm_event_post(GSM_EVT_CMD_ERROR);
            }
            break;

        case AT_CMD_ACK_QUERY:
            ptr = strstr((char *)tempRx.buff, "CLOSE");
            if(ptr != NULL){
                Gsm_event_post(GSM_EVT_CMD_RECONNECT);
                break;
            }
            ptr = strstr((char *)tempRx.buff, "PDP DEACT");
            if(ptr != NULL){
                Gsm_event_post(GSM_EVT_CMD_RECONNECT);
                break;
            }

            ptr = strstr((char *)tempRx.buff, "OK\r\n");
            if (ptr != NULL) {
                ptr = strstr((char *)tempRx.buff, "+QISACK:");
                if (ptr != NULL) {
                    //find first position
                    sentNum = atoi(ptr + sizeof("+QISACK:"));

                    //find second position
                    ptr = strstr((char *)tempRx.buff, ",");
                    ackedNum = atoi(ptr + 1);

                    if((sentNum == ackedNum) && (ackedNum != 0))
                    {
                        //find last position
                        ptr = strrchr((char *)tempRx.buff, ',');
                        if (ptr != NULL) {
                            value = atoi(ptr + 1);
                            if (value == 0)
                                Gsm_event_post(GSM_EVT_CMD_OK);
                        }
                    }
                }
            }
            break;

        case AT_CMD_CLOSE_CONNECT:
            ptr = strstr((char *)tempRx.buff, "CLOSE OK");
            if (ptr != NULL) {
                Gsm_event_post(GSM_EVT_CMD_OK);
                break;
            }
            ptr = strstr((char *)tempRx.buff, "ERROR");
            if (ptr != NULL) {
                Gsm_event_post(GSM_EVT_CMD_ERROR);
            }
            break;

#ifdef USE_QUECTEL_API_FOR_LBS
        case AT_CMD_GET_LOCATION:
            ptr = strstr((char *)tempRx.buff, "ERROR");
            if (ptr != NULL) {
                Gsm_event_post(GSM_EVT_CMD_ERROR);
                break;
            }
            ptr = strstr((char *)tempRx.buff, "OK\r\n");
            if (ptr != NULL) {
                ptr = strstr((char *)tempRx.buff, "+QCELLLOC:");
                if (ptr != NULL) {
                    rGsmObject.location.longitude = atof(ptr + 10);
                    //find last position
                    ptr = strrchr((char *)tempRx.buff, ',');
                    if (ptr != NULL) {
                        latitudetmp = atof(ptr + 1);
                        if(latitudetmp)    
                            rGsmObject.location.latitude = atof(ptr + 1);
                        else {
                            rGsmObject.location.longitude = 0;
                            HIBYTE_ZKS(HIWORD_ZKS(rGsmObject.location.latitude)) = *(ptr + 1);
                            LOBYTE_ZKS(HIWORD_ZKS(rGsmObject.location.latitude)) = *(ptr + 2);
                            HIBYTE_ZKS(LOWORD_ZKS(rGsmObject.location.latitude)) = *(ptr + 3);        
                            LOBYTE_ZKS(LOWORD_ZKS(rGsmObject.location.latitude)) = *(ptr + 4);
                        }
                        Gsm_event_post(GSM_EVT_CMD_OK);
                    }
                }
            }
            break;

#elif defined(USE_ENGINEERING_MODE_FOR_LBS)

        case AT_CMD_ENG_MODE_QUERY:
            ptr =  strstr((char *)tempRx.buff, "OK\r\n");
            if (ptr == NULL)
                break;
            ptr =  strstr((char *)tempRx.buff, "+QENG: 0,");
            if (ptr != NULL) {
                ptr1 = ptr + 9;
                rGsmObject.location.mcc = atoi(ptr1);
                ptr =  strstr(ptr1, ",");
                if (ptr == NULL)
                    break;
                ptr1 = ptr + 1;
                rGsmObject.location.mnc = atoi(ptr1);
                ptr =  strstr(ptr1, ",");
                if (ptr == NULL)
                    break;
                ptr1 = ptr + 1;
                rGsmObject.location.local.lac = htoi(ptr1);
                ptr =  strstr(ptr1, ",");
                if (ptr == NULL)
                    break;
                ptr1 = ptr + 1;
                rGsmObject.location.local.cellid = htoi(ptr1);
                for (i = 0; i < 3; i++) {
                    ptr =  strstr(ptr1, ",");
                    if (ptr == NULL)
                        return;
                    ptr1 = ptr + 1;
                }
                rGsmObject.location.local.dbm = atoi(ptr1);
                if (rGsmObject.location.local.dbm > 0)
                    rGsmObject.location.local.dbm = rGsmObject.location.local.dbm * 2 - 113;
            }
#ifdef SUPPOERT_LBS_NEARBY_CELL
            ptr =  strstr((char *)tempRx.buff, "+QENG: 1,1,");
            if (ptr != NULL) {
                ptr1 = ptr + 11;
                for (index = 0; index < LBS_NEARBY_CELL_MAX; index++) {
                    ptr =  strstr(ptr1, ",");
                    if (ptr == NULL)
                        break;
                    ptr1 = ptr + 1;
                    rGsmObject.location.nearby[index].dbm = atoi(ptr1);
                    if (rGsmObject.location.nearby[index].dbm > 0)
                        rGsmObject.location.nearby[index].dbm = rGsmObject.location.nearby[index].dbm * 2 - 113;

                    for (i = 0; i < 6; i++) {
                        ptr =  strstr(ptr1, ",");
                        if (ptr == NULL)
                            return;
                        ptr1 = ptr + 1;
                    }
                    rGsmObject.location.nearby[index].lac = htoi(ptr1);
                    ptr =  strstr(ptr1, ",");
                    if (ptr == NULL)
                        break;
                    ptr1 = ptr + 1;
                    rGsmObject.location.nearby[index].cellid = htoi(ptr1);
                    ptr =  strstr(ptr1, ",");
                    if (ptr == NULL)
                        break;
                    ptr1 = ptr + 1;
                    ptr =  strstr(ptr1, ",");
                    if (ptr == NULL)
                        break;
                    ptr1 = ptr + 1;
                }
            }
#endif
            Gsm_event_post(GSM_EVT_CMD_OK);
            break;
#endif

#ifdef SUPPORT_IMEI
        case AT_CMD_IMEI_QUERY:
            ptr =  strstr((char *)tempRx.buff, "OK\r\n");
            if (ptr == NULL)
                break;
            memcpy((char *)rGsmObject.imei, (char *)tempRx.buff + 2, 15);
            Gsm_event_post(GSM_EVT_CMD_OK);
            break;
#endif

#ifdef  SUPPORT_NETWORK_SYC_RTC
        case AT_CMD_ENABLE_SYC_RTC:
        case AT_CMD_DISABLE_SYC_RTC:
            ptr =  strstr((char *)tempRx.buff, "OK\r\n");
            if (ptr == NULL)
                break;
            Gsm_event_post(GSM_EVT_CMD_OK);

        case AT_CMD_RTC_QUERY:
            ptr =  strstr((char *)tempRx.buff, "+QLTS:");
            if (ptr == NULL)
                break;
            ptr2 = ptr + 8;
            networkCalendar.Year       = atoi(ptr2)+CALENDAR_BASE_YEAR;
            ptr =  strstr(ptr2, "/");
            if (ptr == NULL)
                break;
            ptr2 = ptr + 1;
            networkCalendar.Month      = atoi(ptr2);
            ptr =  strstr(ptr2, "/");
            if (ptr == NULL)
                break;
            ptr2 = ptr + 1;
            networkCalendar.DayOfMonth = atoi(ptr2);
            ptr =  strstr(ptr2, ",");
            if (ptr == NULL)
                break;
            ptr2 = ptr + 1;
            networkCalendar.Hours      = atoi(ptr2);
            ptr =  strstr(ptr2, ":");
            if (ptr == NULL)
                break;
            ptr2 = ptr + 1;
            networkCalendar.Minutes    = atoi(ptr2);
            ptr =  strstr(ptr2, ":");
            if (ptr == NULL)
                break;
            ptr2 = ptr + 1;
            networkCalendar.Seconds    = atoi(ptr2);
            index = atoi(ptr2+2);
            networkCalendar.Hours += index/4;   // if the timezone less 0, it will wrong
            networkCalendar.Minutes += index%4;
            Gsm_event_post(GSM_EVT_CMD_OK);
            Rtc_set_calendar(&networkCalendar);
            Nwk_Ntp_Set();
            break;

#endif // SUPPORT_NETWORK_SYC_RTC

        default:
            g_rUart1RxData.length = 0;
            ptr = strstr((char *)tempRx.buff, "CLOSE");
            if(ptr != NULL){
                Gsm_event_post(GSM_EVT_CMD_RECONNECT);
                break;
            }
            ptr = strstr((char *)tempRx.buff, "PDP DEACT");
            if(ptr != NULL){
                Gsm_event_post(GSM_EVT_CMD_RECONNECT);
                break;
            }
            break;
    }
}

//***********************************************************************************
//
// Gsm hwi isr callback function.
//
//***********************************************************************************
static void Gsm_hwiIntCallback(void)
{

    if(gsmBusyFlag == 0)
    {

        gsmBusyFlag = 1;
        Clock_start(gsmTimeOutClockHandle);
    }

    gsmTimerFlag = 1;
}

//***********************************************************************************
//
// Gsm event post.
//
//***********************************************************************************
static void Gsm_event_post(UInt event)
{
	if(gsmEvtHandle)
    	Event_post(gsmEvtHandle, event);
}


//***********************************************************************************
//
// Gsm event post.
//
//***********************************************************************************
static void GsmRxToutCb(UArg arg0)
{

    if(gsmTimerFlag == 1)
    {
        gsmTimerFlag = 0;
    }
    else
    {
        if(gsmBusyFlag == 1)
        {
            Clock_stop(gsmTimeOutClockHandle);
            gsmBusyFlag = 0;
            gsmRxLength = g_rUart1RxData.length;
            Swi_post(gsmRxSwiHandle);
        }    
    }
    

}


//***********************************************************************************
//
// Gsm module init.
//
//***********************************************************************************
static void Gsm_init(Nwk_Params *params)
{
    g_rUart1RxData.length = 0;

    Gsm_io_init();

    //Init UART.
    // UartHwInit(UART_0, 38400, Gsm_hwiIntCallback);

    Clock_Params clkParams;
    Clock_Params_init(&clkParams);
    clkParams.period = 0;
    clkParams.startFlag = FALSE;
    Clock_construct(&gsmTimeOutClock, GsmRxToutCb, 1, &clkParams);
    gsmTimeOutClockHandle = Clock_handle(&gsmTimeOutClock);
    Clock_setTimeout(gsmTimeOutClockHandle, 8 * CLOCK_UNIT_MS);
    Clock_setPeriod(gsmTimeOutClockHandle, 8 * CLOCK_UNIT_MS);
    gsmBusyFlag = 0;
    gsmTimerFlag = 0;

    rGsmObject.isOpen = 0;
    rGsmObject.actPDPCnt = 0;
    rGsmObject.uploadFailCnt = 0;
    rGsmObject.sleep = GSM_SLEEP_OFF;
    rGsmObject.resetCnt = 0;
    rGsmObject.state = GSM_STATE_POWEROFF;

    Event_Params eventParam;
    Event_Params_init(&eventParam);
    /* Construct key process Event */
    Event_construct(&gsmEvtStruct, &eventParam);
    /* Obtain event instance handle */
    gsmEvtHandle = Event_handle(&gsmEvtStruct);

    /* Construct a swi Instance to monitor gsm uart */
    Swi_Params swiParams;
    Swi_Params_init(&swiParams);
    swiParams.arg0 = 1;
    swiParams.arg1 = 0;
    swiParams.priority = 1;
    swiParams.trigger = 0;
    Swi_construct(&gsmRxSwiStruct, (Swi_FuncPtr)Gsm_rxSwiFxn, &swiParams, NULL);
    gsmRxSwiHandle = Swi_handle(&gsmRxSwiStruct);

    rGsmObject.dataProcCallbackFxn = params->dataProcCallbackFxn;
}

//***********************************************************************************
//
// Gsm module open.
//
//***********************************************************************************
static uint8_t Gsm_open(void)
{
    if (rGsmObject.isOpen) {
        return FALSE;
    }

    Gsm_poweron();
    rGsmObject.isOpen = 1;
    return TRUE;
}

//***********************************************************************************
//
// Gsm module close.
//
//***********************************************************************************
static uint8_t Gsm_close(void)
{
    if (rGsmObject.isOpen == 0) {
        return FALSE;
    }

    Gsm_poweroff();
    rGsmObject.isOpen = 0;
    return TRUE;
}

//***********************************************************************************
//
// Gsm module control.
//
//***********************************************************************************
static uint8_t Gsm_control(uint8_t cmd, void *arg)
{
    uint8_t result;

	if (cmd == NWK_CONTROL_SIMCCID_GET){//return ccid  whether gsm is on or not
		memcpy((char *)arg, (char *)rGsmObject.simCcid, 20);
		return TRUE;
	}
	
    if (cmd == NWK_CONTROL_RSSI_GET){//return rssi  whether gsm is on or not
        *(uint8_t *)arg = rGsmObject.rssi;
        return TRUE;
    }

    if (rGsmObject.isOpen == 0) {
        return FALSE;
    }

    switch (cmd) {

        case NWK_CONTROL_RSSI_QUERY:
            if (rGsmObject.state > GSM_STATE_GPRS_QUERY && rGsmObject.state <= GSM_STATE_TCP_UPLOAD) {
                result = Gsm_query_csq();
                if (result != RESULT_OK) {
                    rGsmObject.rssi = 0;
                    return FALSE;
                }
            }
            break;


        case NWK_CONTROL_WAKEUP:
            if (rGsmObject.sleep == GSM_SLEEP_ON) {
                result = Gsm_wakeup();
                if (result == RESULT_SHUTDOWN) {
                    return FALSE;
                } else if (result == RESULT_RESET) {
                    Gsm_reset();
                    return FALSE;
                }
                rGsmObject.sleep = GSM_SLEEP_OFF;
            }
            break;

        case NWK_CONTROL_SLEEP:
            if (rGsmObject.sleep == GSM_SLEEP_OFF) {
                result = Gsm_sleep();
                if (result == RESULT_SHUTDOWN) {
                    return FALSE;
                } else if (result == RESULT_RESET) {
                    Gsm_reset();
                    return FALSE;
                }
                rGsmObject.sleep = GSM_SLEEP_ON;
            }
            break;

#ifdef SUPPORT_LBS
        case NWK_CONTROL_LBS_QUERY:
            result = Gsm_transmit_process(NULL, 0, NULL);
            if(result == RESULT_SHUTDOWN || result == RESULT_RESET)
            {
                memset((char *)&rGsmObject.location, 0, sizeof(rGsmObject.location));
                memcpy((char *)arg, (char *)&rGsmObject.location, sizeof(rGsmObject.location));
            }
            if (result == RESULT_SHUTDOWN) {
                return FALSE;
            } else if (result == RESULT_RESET) {
                Gsm_reset();
                return FALSE;
            }
            if (rGsmObject.state == GSM_STATE_TCP_UPLOAD) {
                result = Gsm_get_lbs();
#ifdef USE_QUECTEL_API_FOR_LBS
                if (result != RESULT_OK) {
                    ((NwkLocation_t *)arg)->longitude = 360;
                    ((NwkLocation_t *)arg)->latitude = 360;
                    return FALSE;
                }
#elif defined(USE_ENGINEERING_MODE_FOR_LBS)
                if (result != RESULT_OK) {
                    memset((char *)&rGsmObject.location, 0, sizeof(rGsmObject.location));
                    memcpy((char *)arg, (char *)&rGsmObject.location, sizeof(rGsmObject.location));
                    Gsm_reset();
                    return FALSE;
                }
#endif
                memcpy((char *)arg, (char *)&rGsmObject.location, sizeof(rGsmObject.location));
            }
            break;
#endif

        case NWK_CONTROL_TRANSMIT:
            result = Gsm_transmit_process(((NwkMsgPacket_t *)arg)->buff, ((NwkMsgPacket_t *)arg)->length, 0);
            if (result == RESULT_SHUTDOWN) {
                return FALSE;
            } else if (result == RESULT_RESET) {
                Gsm_reset();
                return FALSE;
            }
            break;
#ifdef SUPPORT_TCP_MULTIL_LINK
        case NWK_CONTROL_TRANSMIT_LINK1:
            result = Gsm_transmit_process(((NwkMsgPacket_t *)arg)->buff, ((NwkMsgPacket_t *)arg)->length, 1);
            if (result == RESULT_SHUTDOWN) {
                return FALSE;
            } else if (result == RESULT_RESET) {
                Gsm_reset();
                return FALSE;
            }
            break;
#endif  //SUPPORT_TCP_MULTIL_LINK

        case NWK_CONTROL_SHUTDOWN_MSG:
            Gsm_event_post(GSM_EVT_SHUTDOWN);
            break;

#ifdef SUPPORT_IMEI
        case NWK_CONTROL_IMEI_GET:
            memcpy((char *)arg, (char *)rGsmObject.imei, 15);
            break;
#endif

        case NWK_CONTROL_TEST:
            result = Gsm_test();
            if (result == RESULT_SHUTDOWN) {
                return FALSE;
            } else if (result == RESULT_RESET) {
                Gsm_reset();
                return FALSE;
            }
            break;
#ifdef SUPPORT_NETWORK_SYC_RTC
        case NWK_CONTROL_SYC_RTC:
            result = Gsm_transmit_process(NULL, 0, NULL);
            if (rGsmObject.state == GSM_STATE_TCP_UPLOAD){
                // result = Gsm_enable_syn_rtc();
                result = Gsm_get_rtc();
                if(result == RESULT_OK)
                    return TRUE;
                return FALSE;
            }
#endif //SUPPORT_NETWORK_SYC_RTC
    }


    return TRUE;
}

//***********************************************************************************
//
// Gsm error indicate.
//
//***********************************************************************************
static void Gsm_error_indicate(void)
{
    switch (rGsmObject.error) {
        case GSM_ERR_UART_SYNC:
            Led_ctrl(LED_R, 1, 250* CLOCK_UNIT_MS, 1);
            break;

        case GSM_ERR_SIM_QUERY:
            Led_ctrl(LED_R, 1, 250* CLOCK_UNIT_MS, 2);
            break;

        case GSM_ERR_CSQ_QUERY:
        case GSM_ERR_CREG_QUERY:
        case GSM_ERR_CGREG_QUERY:
            Led_ctrl(LED_R, 1, 250* CLOCK_UNIT_MS, 3);
            break;

        case GSM_ERR_ACT:
            Led_ctrl(LED_R, 1, 250* CLOCK_UNIT_MS, 4);
            break;

        case GSM_ERR_UPLOAD:
            Led_ctrl(LED_R, 1, 250* CLOCK_UNIT_MS, 5);
            break;

        case GSM_ERR_SLEEP_MODE:
        case GSM_ERR_WAKEUP_MODE:
            Led_ctrl(LED_R, 1, 250* CLOCK_UNIT_MS, 6);
            break;
    }
}


#endif  /* SUPPORT_GSM */

