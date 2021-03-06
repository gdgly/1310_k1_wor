#include "../general.h"

#ifdef SUPPORT_ENGMODE
extern void Lcd_send_cmd(uint8_t cmd);
extern void Lcd_send_data(uint8_t value);
extern Event_Handle systemAppEvtHandle;
extern void Btp_send_cmd(uint8_t *string);
extern void Disp_sensor_data(void);
extern void Disp_icon(uint8_t col, uint8_t row, uint8_t icon, uint8_t light);
//***********************************************************************************
//
// LCD fill full screen.
//
//***********************************************************************************
void Lcd_fill_screen(void)
{
#ifdef LCD_ST7567A
	uint8_t page, col;

	for(page = 0; page < LCD_MAX_PAGE; page++) {
		Lcd_send_cmd(LCD_CMD_PSA + page);					// set the page start address
		Lcd_send_cmd(LCD_CMD_LOWER_SCA + LCD_START_COL);	// set the lower start column address
		Lcd_send_cmd(LCD_CMD_UPPER_SCA);					// set the upper start column address
		for(col = 0; col < LCD_TOTAL_COL; col++)
			Lcd_send_data(0xff);
	}
#endif

#ifdef EPD_GDE0213B1
    EPD_clearScreen();
#endif
}
typedef struct _EngMode_Result
{
	uint16_t  powerkey:1;
	uint16_t  menukey:1;
	uint16_t  lcd:1;
	uint16_t  sensor:1;
	uint16_t  gsm:1;
	uint16_t  bt:1;
	uint16_t  alarm:1;
    uint16_t  lora:1;
	uint16_t  reserve:8;
} EngMode_Result;

EngMode_Result Eng_Result = {0,};

//***********************************************************************************
//
// GetEngModeFlag .
//
//***********************************************************************************
bool GetEngModeFlag()
{
	if ((PIN_getInputValue(Board_BUTTON0) != KEY_PRESSED)
	        && (PIN_getInputValue(Board_BUTTON1) == KEY_PRESSED)
		)
	{
		deviceMode = DEVICES_TEST_MODE;
		return 1;
	}
		
	else
		return 0;
}


extern int8_t RadioCheckRssi(void);

//***********************************************************************************
//
// EngMode.
//
//***********************************************************************************
void EngMode()
{
    uint8_t buff[32];
    uint32_t event;
    
    //close watchdog.    
    // extern Watchdog_Handle watchdogHandle;
    // Watchdog_close(watchdogHandle);  
    
#ifdef SUPPORT_DISP_SCREEN
	g_rSysConfigInfo.status |= STATUS_LCD_ALWAYS_ON;
	Disp_poweron();

	Disp_clear_all();
	Disp_msg(0, 0, "Enter EngMode...", FONT_8X16);
	Disp_msg(0, 2, "Release Power Key", FONT_8X16);
	while(GetEngModeFlag());
	event = Event_pend(systemAppEvtHandle, 0, SYSTEMAPP_EVT_ALL_KEY, BIOS_NO_WAIT);

	//key	
	Disp_clear_all();
		
	Disp_msg(0, 2, "Menu Key:", FONT_8X16);
	event = Event_pend(systemAppEvtHandle, 0, SYSTEMAPP_EVT_ALL_KEY, BIOS_WAIT_FOREVER);
	if (event & SYSTEMAPP_EVT_KEY) {
		Disp_msg(10, 2, "OK", FONT_8X16);
		Eng_Result.menukey = true;
	}
	else {
	
		Disp_msg(10, 2, "Fail", FONT_8X16);
		Eng_Result.menukey = false;
	
	}

	Disp_msg(0, 6, "Press AnyKey ...", FONT_8X16);
	event = Event_pend(systemAppEvtHandle, 0, SYSTEMAPP_EVT_ALL_KEY, BIOS_WAIT_FOREVER);

	//alarm
#if 1
	extern Clock_Handle sysAlarmClkHandle;
	extern void Sys_buzzer_enable(void);
	extern void Sys_buzzer_disable(void);

	Disp_clear_all();
	Disp_msg(0, 0, "Alarm Test:", FONT_8X16);
	Sys_buzzer_enable();
    //Clock_start(sysAlarmClkHandle);

	event = Event_pend(systemAppEvtHandle, 0, SYSTEMAPP_EVT_ALL_KEY, BIOS_WAIT_FOREVER);

	Sys_buzzer_disable();
	//Clock_stop(sysAlarmClkHandle);


	if (event) {
		Eng_Result.alarm = true;
	}
	else {
		Eng_Result.alarm = false;
	}
#endif

		
	//lcd
	Lcd_fill_screen();
	
	event = Event_pend(systemAppEvtHandle, 0, SYSTEMAPP_EVT_ALL_KEY, BIOS_WAIT_FOREVER);
	if (event) {
		Eng_Result.lcd = true;
	}
	else {
	
		Eng_Result.lcd = false;
	
	}


// #ifdef SUPPORT_RADIO
    // if (g_rSysConfigInfo.module & MODULE_CC1310) {

		//lora
		Disp_clear_all();
		Disp_msg(0, 0, "1310 test:", FONT_8X16);	
		int8_t rssi;

        //display rssi
		Lcd_set_font(16, 32, 0);
        while(1){
			//RadioModeSet(RADIOMODE_RECEIVEPORT);
        	// EasyLink_getRssi(&rssi);
            rssi = RadioCheckRssi();
    		if (rssi < 0) {
    	        rssi = -rssi;
    	    }
    	   	Disp_number(3,2,rssi,3,FONT_8X16);
    	   	Task_sleep(500 * CLOCK_UNIT_MS);
    	   	event = Event_pend(systemAppEvtHandle, 0, SYSTEMAPP_EVT_ALL_KEY, BIOS_NO_WAIT);
            if(event !=0)
                break;
        }        
		
		if (event) {
			Eng_Result.lora = true;
		}
		else {
			Eng_Result.lora = false;
		}
    // }
	
// #endif

	Disp_clear_all();
	Disp_poweroff();
    // SysCtrlSystemReset();
#endif

}


#endif// SUPPORT_ENGMODE


