#include "../general.h"


// **************************************************************************
// variable
// rtc timer
static Clock_Struct rtcSecondsClock;     /* not static so you can see in ROV */

static Clock_Handle rtcSecondsClockHandle;


// semaphore 


// rtc
static Calendar rtc;

static void (*RtcSecIsbCB)(void);

// *******************************************************************************
// *Funtion name: MonthMaxDay
// *Description :
// *
// *Input: the month and year
// *Output:the last day of the month
// /******************************************************************************
uint8_t MonthMaxDay(uint16_t year_1, uint8_t month_1)
{
    uint8_t maxday;

    // leap year judgement
    if((year_1%4) == 0)
    {
        if((year_1 % 100) != 0)
            maxday = 29;
        else
        {
            if((year_1%400) == 0)
                maxday  = 29;
            else
                maxday  = 28;
        }
    }
    else
    {
        maxday = 28;
    }
    
    switch(month_1)
    {
        case 1:
            return(31);
        case 2:
            return(maxday);
        case 3:
            return(31);
        case 4:
            return (30);
        case 5:
            return(31);
        case 6:
            return(30);
        case 7:
            return(31);
        case 8:
            return (31);
        case 9:
            return(30);
        case 10:
            return(31);
        case 11:
            return(30);
        case 12:
            return (31);
        default:
            break;
    }
        return 0xff;
}

static uint8_t key_cnt  = 0;
void RtcSecondsIsrCb(UArg arg0)
{
    rtc.Seconds++;
    if(rtc.Seconds >= 60){
        rtc.Seconds = 0;
        rtc.Minutes++;
        Sys_event_post(SYSTEMAPP_EVT_DISP);
        if(rtc.Minutes >= 60){
            rtc.Minutes = 0;
            rtc.Hours++;
            if(rtc.Hours >= 24){
                rtc.Hours = 0;
                rtc.DayOfMonth++;
                rtc.DayOfWeek++;
                if(rtc.DayOfWeek > 6)
                    rtc.DayOfWeek = 0;

                if(rtc.DayOfMonth > MonthMaxDay(rtc.Year, rtc.Month)){
                    rtc.DayOfMonth = 1;
                    rtc.Month++;
                    if(rtc.Month >12){
                        rtc.Month = 1;
                        rtc.Year ++;
                    }
                }
            }
        }
    }
#ifdef S_G
    if(get_Key_cnt()!=0)
    {

        if(key_cnt > 1)
            set_key_cnt_zero();
        key_cnt++;
    }
    else
        key_cnt = 0;
#endif //S_G

    RtcSecIsbCB();
}


void RtcAdjust(Calendar *calendar)
{
    while(calendar->Seconds >= 60)
    {
        calendar->Seconds -= 60;
        calendar->Minutes++;
    }
    while(calendar->Minutes >= 60)
    {
        calendar->Minutes -= 60;
        calendar->Hours++;
    }
    while(calendar->Hours>=24)
    {
        calendar->Hours -= 24;
        rtc.DayOfMonth++;
    }
    while(rtc.DayOfMonth > MonthMaxDay(rtc.Year, rtc.Month))
    {
        rtc.DayOfMonth -= MonthMaxDay(rtc.Year, rtc.Month);
        rtc.Month++;
        if(rtc.Month > 12)
            rtc.Month = 1;
            rtc.Year ++;
    }
}


//***********************************************************************************
//
// RTC init Calendar Mode.
//
//***********************************************************************************
void RtcInit(void (*Cb)(void))
{

    /* Create clock object which is used for fast report timeout */
    Clock_Params clkParams;
    Clock_Params_init(&clkParams);
    clkParams.period = CLOCK_UNIT_S;
    clkParams.startFlag = FALSE;
    Clock_construct(&rtcSecondsClock, RtcSecondsIsrCb, 1, &clkParams);
    rtcSecondsClockHandle = Clock_handle(&rtcSecondsClock);

#ifdef S_G
    rtc = read_time_from_sd30xx();
#endif //S_G
    /*
    rtc.Year       = g_rSysConfigInfo.rtc.Year;
    rtc.Month      = 1;
    rtc.DayOfMonth = 1;
    rtc.DayOfWeek  = 1;
    rtc.Hours      = 0;
    rtc.Minutes    = 0;
    rtc.Seconds    = 0;
    */


    RtcSecIsbCB = Cb;
}

//***********************************************************************************
//
// RTC start.
//
//***********************************************************************************
void RtcStart(void)
{
    if(Clock_isActive(rtcSecondsClockHandle) == false)
        Clock_start(rtcSecondsClockHandle);
}

//***********************************************************************************
//
// RTC stop.
//
//***********************************************************************************
void RtcStop(void)
{
    if(Clock_isActive(rtcSecondsClockHandle))
        Clock_stop(rtcSecondsClockHandle);
}




//***********************************************************************************
//
// RTC set Calendar time.
//
//***********************************************************************************
void Rtc_set_calendar(Calendar *currentTime)
{
    UInt key;
    key = Hwi_disable();
    memcpy(&rtc, currentTime, sizeof(Calendar));
#ifdef S_G
    write_time_to_sd30xx(rtc);
#endif //S_G
    Hwi_restore(key);
}

//***********************************************************************************
//
// RTC get Calendar time.
//
//***********************************************************************************
Calendar Rtc_get_calendar(void)
{
    return rtc;
}


//***********************************************************************************
//
// RTC get Calendar time.
//
//***********************************************************************************
uint8_t RtcGetSec(void)
{
    return rtc.Seconds;
}
