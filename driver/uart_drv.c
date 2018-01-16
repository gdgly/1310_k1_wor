/*
* @Author: zxt
* @Date:   2017-12-21 17:36:18
* @Last Modified by:   zxt
* @Last Modified time: 2018-01-16 10:16:16
*/
#include "../general.h"

/* UART Board */
#define CC1310_LAUNCHXL_UART_RX               IOID_5          /* RXD */
#define CC1310_LAUNCHXL_UART_TX               IOID_4          /* TXD */
#define CC1310_LAUNCHXL_UART_CTS              IOID_19         /* CTS */
#define CC1310_LAUNCHXL_UART_RTS              IOID_18         /* RTS */

/*
 *  =============================== UART ===============================
 */
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>
#include <ti/drivers/power/PowerCC26XX.h>

UARTCC26XX_Object uartCC26XXObjects[CC1310_LAUNCHXL_UARTCOUNT];

const UARTCC26XX_HWAttrsV2 uartCC26XXHWAttrs[CC1310_LAUNCHXL_UARTCOUNT] = {
    {
        .baseAddr       = UART0_BASE,
        .powerMngrId    = PowerCC26XX_PERIPH_UART0,
        .intNum         = INT_UART0_COMB,
        .intPriority    = ~0,
        .swiPriority    = 0,
        .txPin          = CC1310_LAUNCHXL_UART_TX,
        .rxPin          = CC1310_LAUNCHXL_UART_RX,
        .ctsPin         = PIN_UNASSIGNED,
        .rtsPin         = PIN_UNASSIGNED
    }
};

const UART_Config UART_config[CC1310_LAUNCHXL_UARTCOUNT] = {
    {
        .fxnTablePtr = &UARTCC26XX_fxnTable,
        .object      = &uartCC26XXObjects[CC1310_LAUNCHXL_UART0],
        .hwAttrs     = &uartCC26XXHWAttrs[CC1310_LAUNCHXL_UART0]
    },
};


const uint_least8_t UART_count = CC1310_LAUNCHXL_UARTCOUNT;



UartRxData_t     uart0RxData;
UartRxData_t     uart0IsrRxData;
UartRxData_t     uart0TxData;


/***************************************************************************/

static UART_Handle      uarthandle[CC1310_LAUNCHXL_UARTCOUNT];
static uint8_t recBuf[32];          //1310 hardware uart rec len is 16 which take from testing
static void (*Uart0IsrCb)(uint8_t *databuf, uint8_t len);
void uart0Isr(void)
{
    uint32_t               intStatus;
    uint8_t                 i;
    i = 0;
    intStatus       = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE,intStatus);

    if(intStatus & UART_INT_RX || intStatus & UART_INT_RT)
    {
        while(1)
        if(!(HWREG(UART0_BASE + UART_O_FR) & UART_FR_RXFE))
        {
            // Read and return the next character.

            recBuf[i] = (HWREG(UART0_BASE + UART_O_DR));
            i++;
        }
        else
        {
            break;
        }
        Uart0IsrCb(recBuf, i);
    }
}


uint8_t testkk[20] = "text zxt";
//***********************************************************************************
//
// UART0/1 init, use USCI_A0/1.
//
//***********************************************************************************
void UartHwInit(UART_PORT uartPort, uint32_t baudrate, UART_CB_T Cb)
{

    if (uartPort >= UART_MAX) {
        return;
    }


    UART_init();

    UART_Params uartParams;

    UART_Params_init(&uartParams);

    uartParams.readMode       = UART_MODE_CALLBACK;
    uartParams.writeMode      = UART_MODE_BLOCKING;
    uartParams.readTimeout    = UART_WAIT_FOREVER;
    uartParams.writeTimeout   = UART_WAIT_FOREVER;
    uartParams.readCallback   = (UART_Callback)uart0Isr;
    uartParams.writeCallback  = NULL;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readDataMode   = UART_DATA_BINARY;
    uartParams.writeDataMode  = UART_DATA_BINARY;
    uartParams.readEcho       = UART_ECHO_OFF;
    uartParams.baudRate       = baudrate;
    uartParams.dataLength     = UART_LEN_8;
    uartParams.stopBits       = UART_STOP_ONE;
    uartParams.parityType     = UART_PAR_NONE;

    Uart0IsrCb                = Cb;

    uarthandle[uartPort]      = UART_open(CC1310_LAUNCHXL_UART0, &uartParams);

    // enable the uart int
    UART_read(uarthandle[CC1310_LAUNCHXL_UART0], (uint8_t *)&baudrate, 1);

}

void UartClose(UART_PORT uartPort)
{
    if (uartPort >= UART_MAX) {
        return;
    }

    UART_readCancel(uarthandle[CC1310_LAUNCHXL_UART0]);
    UART_close(uarthandle[uartPort]);
}



void UartSendDatas(UART_PORT uartPort, uint8_t *buf, uint8_t count)
{
    if (uartPort >= UART_MAX) {
        return;
    }
    UART_write(uarthandle[uartPort], buf, count);
}


//***********************************************************************************
//
// UART0/1 send burst byte, use USCI_A0/1.
//
//***********************************************************************************
void Uart_send_burst_data(UART_PORT uartPort, uint8_t *pData, uint16_t length)
{
    uint16_t i;

    for (i = 0; i < length; i++) {
        UART_write(uarthandle[uartPort], pData+i, 1);
    }
}

//***********************************************************************************
//
// UART0/1 send string, use USCI_A0/1.
//
//***********************************************************************************
void Uart_send_string(UART_PORT uartPort, uint8_t *string)
{
    while (*string != '\0') {
        UART_write(uarthandle[uartPort], string++, 1);
    }
}


//***********************************************************************************
//
// UART0/1 Close.
//
//***********************************************************************************

