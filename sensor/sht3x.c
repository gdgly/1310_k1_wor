//***********************************************************************************
// Copyright 2017, Zksiot Development Ltd.
// Created by Linjie, 2017.08.08
// MCU:	MSP430F5529
// OS: TI-RTOS
// Project:
// File name: sht2x.c
// Description: SHT2x humiture sensor process routine.
//***********************************************************************************
#include "../general.h"







#ifdef SUPPORT_SHT3X

#include "sht3x.h"

//***********************************************************************************
//
// sort data.
//
//***********************************************************************************
static void sort_data(int16_t *data, uint16_t len)
{
    uint16_t i , j;
    int16_t temp;

    for (i = 0; i < (len - 1); i++)
    {
        for (j = 0; j < (len - 1 - i); j++)
            if (data[j] > data[j+1])
            {
                  temp = data[j];
                  data[j] = data[j+1];
                  data[j+1] = temp;
            }
    }
}

static PIN_State   sht3xResetPinState;
static PIN_Handle  sht3xResetPinHandle;


#define SHT3X_RESET_PIN        IOID_26

#ifdef BOARD_B2S
#undef SHT3X_RESET_PIN
#define SHT3X_RESET_PIN       IOID_23
#endif

const PIN_Config sht3xResetPinTable[] = {
    SHT3X_RESET_PIN | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,       /* LED initially off          */
    PIN_TERMINATE
};

#define SH3X_RESET_SET         PIN_setOutputValue(sht3xResetPinHandle, SHT3X_RESET_PIN, 1)
#define SH3X_RESET_CLR         PIN_setOutputValue(sht3xResetPinHandle, SHT3X_RESET_PIN, 0)


//***********************************************************************************
//
// SHT3x_ResetIoInitial
//
//***********************************************************************************

void SHT3x_ResetIoInitial(void){
    sht3xResetPinHandle = PIN_open(&sht3xResetPinState, sht3xResetPinTable);
}



//***********************************************************************************
//
// SHT3x check crc.
//
//***********************************************************************************
static ErrorStatus SHT3x_check_crc(uint8_t *pData, uint8_t length, uint8_t checksum)
{
    uint8_t i, j, crc = 0xFF;

    //calculates 8-Bit checksum with given polynomial
    for (i = 0; i < length; i++) {
        crc ^= pData[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }

    if (crc != checksum)
        return ES_ERROR;
    else
        return ES_SUCCESS;
}


//***********************************************************************************
//
// SHT3x calculate temperature degree Celsius. x100.
//
//***********************************************************************************
static int16_t SHT3x_calc_temperatureC(uint16_t temperature)
{
    int16_t temperatureC;

    if (temperature == 0)
        return TEMPERATURE_OVERLOAD;

    //-- calculate temperature [??] --
    // T = -45 + 175 * rawValue / (2^16-1)
    temperatureC= (int16_t)round((-45 + 175 *(float)temperature / 65535.0f)*100 );     //175.0f * (ft)rawValue / 65535.0f - 45.0f;
    return temperatureC;
}





//***********************************************************************************
//
// SHT2x calculate humidty. x100
//
//***********************************************************************************
static uint16_t SHT3x_calc_humidty(uint16_t humidty)
{
    uint16_t humidity;

    if (humidty == 0)
        return HUMIDTY_OVERLOAD;

    //-- calculate relative humidity [%RH] --   100.0f * (ft)rawValue / 65535.0f;
    humidity =   (uint16_t)round((100.0f * (float)humidty / 65535.0f)*100);             //(uint16_t)round(-6.0 + 125.0/65536 * (float)humidty * 100); 100.0f * (ft)rawValue / 65535.0f;

    if(humidity > 10000)//??humidity?T???ú100
        humidity = 10000;

    return humidity;
}




//***********************************************************************************
//
// SHT3x init.
//
//***********************************************************************************
static void SHT3x_init(uint8_t chNum)
{
    uint8_t buff[2];
    uint8_t err=0;
    uint16_t command_word;

    if (g_rSysConfigInfo.sensorModule[chNum] == SEN_TYPE_SHT2X){
        // && rSensorHWAttrs[chNum].chNum < SEN_I2C_MAX) {
#if 1
        //硬件复位脚直接复位芯片
        SH3X_RESET_CLR;
        delay_ms(10); //复位拉低时间至少1US
        SH3X_RESET_SET;

#else
        command_word = CMD_SOFT_RESET;
        buff[0] = HIBYTE(command_word);
        buff[1] = LOBYTE(command_word);
       // I2c_write(Board_SHT3x_ADDR, buff, 2);
        I2C_start();
        I2C_send_byte(Board_SHT3x_ADDR<<1);
        if(I2C_ack_receive()==false){
            err |= 0x01;
        }
        I2C_send_byte(buff[0]);
        if(I2C_ack_receive()==false){
            err |= 0x01;
        }
        I2C_send_byte(buff[1]);
        if(I2C_ack_receive()==false){
            err |= 0x01;
        }
        I2C_stop();
#endif
        Task_sleep(50 * CLOCK_UNIT_MS);

    }
}


//===========================================================================
//===========================================================================
uint8_t SHT3x_ReadUserRegister(uint16_t *pRegisterValue)
{
    uint8_t error=0; //variable for error code
    uint8_t buff[3];


       buff[0] =  HIBYTE_ZKS(*pRegisterValue);
       buff[1] =  LOBYTE_ZKS(*pRegisterValue);
      // I2c_read(Board_SHT3x_ADDR<<1, buff, 3);
       I2C_start();
       I2C_send_byte (Board_SHT3x_ADDR<<1);
       if(I2C_ack_receive() == false){
            error |= 0x01;
       }
      I2C_send_byte ( buff[0]);
      if(I2C_ack_receive() == false){
            error |= 0x01;
      }
      I2C_send_byte ( buff[1]);
      if(I2C_ack_receive() == false){
            error |= 0x01;
      }
      I2C_start();
      I2C_send_byte((Board_SHT3x_ADDR<<1) | 0x01);
      if(I2C_ack_receive() == false){
            error |= 0x01;
      }
      buff[0] = I2C_receive_byte();
      I2C_ack_send();
      buff[1] = I2C_receive_byte();
      I2C_ack_send();
      buff[2] = I2C_receive_byte();
      I2C_noack_send();
      I2C_stop();
      if( SHT3x_check_crc(buff, 2, buff[2])!=ES_SUCCESS){
          error |= 0x01;
      }
      else if(buff[1]&0x02){ //last command not processed
          error |= 0x01;
     }
    return error;
}


//===========================================================================
//===========================================================================
uint8_t SHT3x_WriteUserRegister(uint16_t *pRegisterValue)
{
    uint8_t error=0; //variable for error code
    uint8_t buff[2];
    buff[0] = HIBYTE_ZKS(*pRegisterValue);
    buff[1] = LOBYTE_ZKS(*pRegisterValue);
    I2C_start();
    I2C_send_byte(Board_SHT3x_ADDR<<1);
    if(I2C_ack_receive()==false){
         error |= 0x01;
    }
    I2C_send_byte(buff[0]);
    if(I2C_ack_receive()==false){
        error |= 0x01;
    }
    I2C_send_byte(buff[1]);
    if(I2C_ack_receive()==false){
        error |= 0x01;
    }
    I2C_stop();

 return(error);

}


//===========================================================================
uint8_t SHT3x_MeasurePoll(uint8_t eSHT2xMeasureType, uint8_t *pMeasurand)
//===========================================================================
{

    uint8_t error=0; //error variable
    uint16_t command_word=0; //counting variable

    command_word = CMD_MEAS_POLLING_H;  //2aá?
    SHT3x_WriteUserRegister(&command_word);

    Task_sleep(50 * CLOCK_UNIT_MS);     //μè′y50ms ???èó?êa?èá???2?êy??2aá?

   // I2c_read(Board_SHT3x_ADDR<<1, buff, 6);
    I2C_start();
    I2C_send_byte ((Board_SHT3x_ADDR<<1)|0x01);
    if(I2C_ack_receive() == false){
        error |= 0x01;
    }

    pMeasurand[0] = I2C_receive_byte();  //temperature
    I2C_ack_send();
    pMeasurand[1] = I2C_receive_byte();
    I2C_ack_send();
    pMeasurand[2] = I2C_receive_byte();
    I2C_ack_send();

    pMeasurand[3] = I2C_receive_byte(); //humidity
    I2C_ack_send();
    pMeasurand[4] = I2C_receive_byte();
    I2C_ack_send();
    pMeasurand[5] = I2C_receive_byte();
    I2C_noack_send();
    I2C_stop();

    if (SHT3x_check_crc(pMeasurand, 2, pMeasurand[2]) != ES_SUCCESS){
        error |= 0x01;
    }

    if (SHT3x_check_crc(&pMeasurand[3], 2, pMeasurand[5]) != ES_SUCCESS){
        error |= 0x01;
    }


    return error;
}


//***********************************************************************************
//
// SHT3x measure.
//
//***********************************************************************************
static void SHT3x_measure(uint8_t chNum)
{
    uint16_t command_word;
    uint8_t buff[6];
    uint8_t error=0;
   // uint8_t i;
    //int16_t tempT[5], humiT[5], delt;
    uint8_t retrys = 0;//3?′íê±??ê?3′?
    error = 0;
   // i = 0;
    if (g_rSysConfigInfo.sensorModule[chNum] == SEN_TYPE_SHT2X){
        //&& rSensorHWAttrs[chNum].chNum < SEN_I2C_MAX) {

       // GPIO_setOutputHighOnPin(rSensorHWAttrs[chNum].port, rSensorHWAttrs[chNum].pin);
        if (g_rSysConfigInfo.sensorModule[chNum] == SEN_TYPE_SHT2X) {
//measure_retrys:
             retrys = 0;
             SHT3x_init(chNum);
err_retrys:
             command_word = CMD_READ_STATUS;
             error |= SHT3x_ReadUserRegister(&command_word);
             error |= SHT3x_MeasurePoll(0, buff);
            if (error == 0) {
                  HIBYTE_ZKS(rSensorData[chNum].temp) = buff[0];  // temperature
                  LOBYTE_ZKS(rSensorData[chNum].temp) = buff[1];

                  HIBYTE_ZKS(rSensorData[chNum].humi) = buff[3]; // humidity
                  LOBYTE_ZKS(rSensorData[chNum].humi) = buff[4];
             }
            else {
                  rSensorData[chNum].temp = 0;
                  rSensorData[chNum].humi = 0;
                  if(retrys++ < 3)goto err_retrys;
             }
            //convert rawdata to temperature
            rSensorData[chNum].temp = SHT3x_calc_temperatureC(rSensorData[chNum].temp );
            //convert rawdata to humidty
            rSensorData[chNum].humi= SHT3x_calc_humidty(rSensorData[chNum].humi);


#if 0
            tempT[i] = rSensorData[chNum].temp;
            humiT[i] = rSensorData[chNum].humi;

            i++;
            if(i == 2)
            {
                delt = tempT[0] - tempT[1];
                if(delt < 0)
                    delt = -delt;

                if(delt > 200)
                    goto measure_retrys;

                delt = humiT[0] - humiT[1];
                if(delt < 0)
                    delt = -delt;

                if(delt > 2000)
                    goto measure_retrys;

            }

            if(i == 5)
            {
                sort_data(tempT, 5);
                sort_data(humiT, 5);
                rSensorData[chNum].temp = tempT[2];
                rSensorData[chNum].humi = humiT[2];
            }
            if(i != 2 && i < 5)
            {
                goto measure_retrys;
            }
#endif
        }

     //GPIO_setOutputLowOnPin(rSensorHWAttrs[chNum].port, rSensorHWAttrs[chNum].pin);
    }
}

//***********************************************************************************
//
// SHT3x calculate humidty.
//
//***********************************************************************************
static int32_t SHT3x_get_value(uint8_t chNum, SENSOR_FUNCTION function)
{
    if (g_rSysConfigInfo.sensorModule[chNum] == SEN_TYPE_SHT2X){
        //&& rSensorHWAttrs[chNum].chNum < SEN_I2C_MAX) {

        if(function & SENSOR_TEMP){
            return rSensorData[chNum].temp;
        } else if(function & SENSOR_HUMI){
            return rSensorData[chNum].humi;
        }
    }

    return TEMPERATURE_OVERLOAD;
}


const Sensor_FxnTable  SHT3X_FxnTable = {
    SENSOR_TEMP | SENSOR_HUMI,
    SHT3x_init,
    SHT3x_measure,
    SHT3x_get_value,
};


#endif/* SUPPORT_SHT3X */












