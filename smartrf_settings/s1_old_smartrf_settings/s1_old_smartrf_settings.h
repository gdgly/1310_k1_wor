#ifndef _S1_OLD_SMARTRF_SETTINGS_H_
#define _S1_OLD_SMARTRF_SETTINGS_H_


//*********************************************************************************
// Generated by SmartRF Studio version 2.8.0 ( build #41)
// Compatible with SimpleLink SDK version: CC13x0 SDK 1.30.xx.xx
// Device: CC1310 Rev. 2.1 (Rev. B)
// 
//*********************************************************************************

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_prop_cmd.h)
#include <ti/drivers/rf/RF.h>


// TI-RTOS RF Mode Object
extern RF_Mode RF_prop_s1_old;


// RF Core API commands
extern  rfc_CMD_PROP_RADIO_DIV_SETUP_t RF_cmdPropRadioDivSetup_s1_old;
extern  rfc_CMD_FS_t RF_cmdFs_s1_old;
extern  rfc_CMD_PROP_RX_t RF_cmdPropRx_s1_old;
extern  rfc_CMD_PROP_TX_t RF_cmdPropTx_s1_old;
extern rfc_CMD_PROP_RX_ADV_t RF_cmdPropRxAdv_preDef_s1_old;


#endif // _S1_OLD_SMARTRF_SETTINGS_H_

