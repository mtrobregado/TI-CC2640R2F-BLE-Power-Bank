/******************************************************************************

 @file       ble_powerbank.c

 @brief This file contains the BLE Power Bank sample application for use
        with the CC2640R2F Bluetooth Low Energy Protocol Stack.

 Group: CMCU, SCS
 Target Device: CC2640R2

 ******************************************************************************
 
 Copyright (c) 2013-2017, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 Release Name: simplelink_cc2640r2_sdk_01_50_00_58
 Release Date: 2017-10-17 18:09:51

 Project: BLE Power Bank
 Maker/Author - Markel T. Robregado
 Date: June 15, 2018
 Modification Details : Add BOOSTXL-BATPAKMKII to CC2640R2F Launchpad to function
                        as BLE Power Bank
 Device Setup: TI CC2640R2F Launchpad + BOOSTXL-BATPAKMKII + 430BOOST-SHARP96 LCD
               + MOSFET Switch Board
 *****************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/display/Display.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <driverlib/sys_ctrl.h>
#ifdef WATCHDOG_EN
#include <ti/drivers/Watchdog.h>
#include "hal_mcu.h"
#endif

#if defined( USE_FPGA ) || defined( DEBUG_SW_TRACE )
#include <driverlib/ioc.h>
#endif // USE_FPGA | DEBUG_SW_TRACE

#include <icall.h>
#include "util.h"
/* This Header file contains all BLE API and icall structure definition */
#include "icall_ble_api.h"
#ifdef DEV_INFO
#include "devinfoservice.h"
#endif
#include "LiPoBattService.h"

#include "ll_common.h"

#include "peripheral.h"

#ifdef USE_RCOSC
#include "rcosc_calibration.h"
#endif //USE_RCOSC

#include "Board.h"
#include "HAL_BQ27441.h"
#include "HAL_I2C.h"

#include <ble_powerbank.h>

/*********************************************************************
 * CONSTANTS
 */

// Advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL          160

// General discoverable mode: advertise indefinitely
#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL

// Minimum connection interval (units of 1.25ms, 80=100ms) for automatic
// parameter update request
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     80

// Maximum connection interval (units of 1.25ms, 800=1000ms) for automatic
// parameter update request
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     800

// Slave latency to use for automatic parameter update request
#define DEFAULT_DESIRED_SLAVE_LATENCY         0

// Supervision timeout value (units of 10ms, 1000=10s) for automatic parameter
// update request
#define DEFAULT_DESIRED_CONN_TIMEOUT          1000

// After the connection is formed, the peripheral waits until the central
// device asks for its preferred connection parameters
#define DEFAULT_ENABLE_UPDATE_REQUEST         GAPROLE_LINK_PARAM_UPDATE_WAIT_REMOTE_PARAMS

// Connection Pause Peripheral time value (in seconds)
#define DEFAULT_CONN_PAUSE_PERIPHERAL         6

// How often to perform periodic event (in msec)
#define BLE_PERIODIC_EVT_PERIOD               30000
#define BQ27441_READ_EVT_PERIOD               5000

// Application specific event ID for HCI Connection Event End Events
#define SBP_HCI_CONN_EVT_END_EVT              0x0001

// Type of Display to open
#if !defined(Display_DISABLE_ALL)
    #if defined(BOARD_DISPLAY_USE_LCD) && (BOARD_DISPLAY_USE_LCD!=0)
        #define SBP_DISPLAY_TYPE Display_Type_LCD
    #elif defined (BOARD_DISPLAY_USE_UART) && (BOARD_DISPLAY_USE_UART!=0)
        #define SBP_DISPLAY_TYPE Display_Type_UART
    #else // !BOARD_DISPLAY_USE_LCD && !BOARD_DISPLAY_USE_UART
        #define SBP_DISPLAY_TYPE 0 // Option not supported
    #endif // BOARD_DISPLAY_USE_LCD && BOARD_DISPLAY_USE_UART
#else // BOARD_DISPLAY_USE_LCD && BOARD_DISPLAY_USE_UART
    #define SBP_DISPLAY_TYPE 0 // No Display
#endif // !Display_DISABLE_ALL

// Task configuration
#define SBP_TASK_PRIORITY                     1

#ifndef SBP_TASK_STACK_SIZE
#define SBP_TASK_STACK_SIZE                   644
#endif

// Application events
#define SBP_STATE_CHANGE_EVT                  0x0001
#define SBP_CHAR_CHANGE_EVT                   0x0002
#define SBP_PAIRING_STATE_EVT                 0x0004
#define SBP_PASSCODE_NEEDED_EVT               0x0008

// Internal Events for RTOS application
#define SBP_ICALL_EVT                         ICALL_MSG_EVENT_ID // Event_Id_31
#define SBP_QUEUE_EVT                         UTIL_QUEUE_EVENT_ID // Event_Id_30
#define SBP_PERIODIC_EVT                      Event_Id_00
#define SBP_CHA_DET_EVT                       Event_Id_01
#define SBP_BQ27441_EVT                       Event_Id_02

// Bitwise OR of all events to pend on
#define SBP_ALL_EVENTS                        (SBP_ICALL_EVT        | \
                                               SBP_QUEUE_EVT        | \
                                               SBP_PERIODIC_EVT     | \
                                               SBP_CHA_DET_EVT      | \
                                               SBP_BQ27441_EVT)

#define SNV_ID_LIPOBATTHI 0x80
#define SNV_ID_LIPOBATTCRIT 0x81
#define LIPO_BATTHI   95
#define LIPO_BATTCRIT 25

/*********************************************************************
 * TYPEDEFS
 */

// App event passed from profiles.
typedef struct
{
    appEvtHdr_t hdr;  // event header.
    uint8_t *pData;  // event data
} sbpEvt_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Display Interface
Display_Handle dispHandle = NULL;

/* Wake-up Button pin table */
PIN_Config ButtonTableWakeUp[] =
{
    Board_PIN_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PINCC26XX_WAKEUP_NEGEDGE,
    BATPAKMKII_BP_CHA_PIN | PIN_INPUT_EN | PIN_PULLUP | PINCC26XX_WAKEUP_NEGEDGE,
    PIN_TERMINATE                                 /* Terminate list */
};

/*
 * BLE Power Bank pin configuration table:
 */
PIN_Config BLEPowerBankPinTable[] =
{
     Board_PIN_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
     BATPAKMKII_BP_CHA_PIN | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,
     Board_MOS_TRIG | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_DRVSTR_MIN,
     PIN_TERMINATE
};

gaprole_States_t GapProfileState = GAPROLE_INIT;

/*********************************************************************
 * LOCAL VARIABLES
 */

// Entity ID globally used to check for source and/or destination of messages
static ICall_EntityID selfEntity;

// Event globally used to post local events and pend on system and
// local events.
static ICall_SyncHandle syncEvent;

// Clock instances for internal periodic events.
static Clock_Struct periodicClock;
static Clock_Struct bq27441readClock;

// Queue object used for app messages
static Queue_Struct appMsg;
static Queue_Handle appMsgQueue;

// Task configuration
Task_Struct sbpTask;
Char sbpTaskStack[SBP_TASK_STACK_SIZE];

// Scan response data (max size = 31 bytes)
static uint8_t scanRspData[] =
{
    // complete name
    0x14,   // length of this data
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'B',
    'L',
    'E',
    ' ',
    'P',
    'o',
    'w',
    'e',
    'r',
    ' ',
    'B',
    'a',
    'n',
    'k',

    // connection interval range
    0x05,   // length of this data
    GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
    LO_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),   // 100ms
    HI_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
    LO_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),   // 1s
    HI_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),

    // Tx power level
    0x02,   // length of this data
    GAP_ADTYPE_POWER_LEVEL,
    0       // 0dBm
};

// Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertising)
static uint8_t advertData[] =
{
  // Flags: this field sets the device to use general discoverable
  // mode (advertises indefinitely) instead of general
  // discoverable mode (advertise for 30 seconds at a time)
  0x02,   // length of this data
  GAP_ADTYPE_FLAGS,
  DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

  // service UUID, to notify central devices what services are included
  // in this peripheral
  0x03,   // length of this data
  GAP_ADTYPE_16BIT_MORE,      // some of the UUID's, but not all
  LO_UINT16(LIPOBATTSERVICE_SERV_UUID),
  HI_UINT16(LIPOBATTSERVICE_SERV_UUID)
};

// GAP GATT Attributes
static uint8_t attDeviceName[GAP_DEVICE_NAME_LEN] = "BLE Power Bank";

// Globals used for ATT Response retransmission
static gattMsgEvent_t *pAttRsp = NULL;
static uint8_t rspTxRetry = 0;

/* Pin driver handles */
static PIN_Handle blePowerBankPinHandle;

/* Pin states */
static PIN_State blePowerBankPinState;

#ifdef WATCHDOG_EN
Watchdog_Handle WatchdogHandle;
#endif

static uint8_t chargePinFlag = 0;
static short bq27441charge, bq27441voltage, bq27441remcap, bq27441current, bq27441temp = 0;
static short batthi, battcrit = 0;
static uint8_t notval = 0;
static uint8_t hidischarge_flag = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void BLE_PowerBank_init(void);
static void BLE_PowerBank_taskFxn(UArg a0, UArg a1);

static uint8_t BLE_PowerBank_processStackMsg(ICall_Hdr *pMsg);
static uint8_t BLE_PowerBank_processGATTMsg(gattMsgEvent_t *pMsg);
static void BLE_PowerBank_processAppMsg(sbpEvt_t *pMsg);
static void BLE_PowerBank_processStateChangeEvt(gaprole_States_t newState);
static void BLE_PowerBank_processCharValueChangeEvt(uint8_t paramID);
static void BLE_PowerBank_Shutdown(void);
static void BLE_PowerBank_clockHandler(UArg arg);

static void BLE_PowerBank_sendAttRsp(void);
static void BLE_PowerBank_freeAttRsp(uint8_t status);

static void BLE_PowerBank_passcodeCB(uint8_t *deviceAddr, uint16_t connHandle,
                                           uint8_t uiInputs, uint8_t uiOutputs);
static void BLE_PowerBank_pairStateCB(uint16_t connHandle, uint8_t state,
                                         uint8_t status);
static void BLE_PowerBank_processPairState(uint8_t state, uint8_t status);
static void BLE_PowerBank_processPasscode(uint8_t uiOutputs);

static void BLE_PowerBank_stateChangeCB(gaprole_States_t newState);
static uint8_t BLE_PowerBank_enqueueMsg(uint8_t event, uint8_t state,
                                              uint8_t *pData);
static void BLE_PowerBank_ReadBQ27441(void);
#ifdef WATCHDOG_EN
static void Watchdog_Init(void);
#endif
static void BLE_PowerBank_LiPoBattServiceValueChangeCB(uint8_t paramID); // Callback from the service.

/*********************************************************************
 * EXTERN FUNCTIONS
 */
extern void AssertHandler(uint8 assertCause, uint8 assertSubcause);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// Peripheral GAPRole Callbacks
static gapRolesCBs_t BLE_PowerBank_gapRoleCBs =
{
    BLE_PowerBank_stateChangeCB     // GAPRole State Change Callbacks
};

// GAP Bond Manager Callbacks
// These are set to NULL since they are not needed. The application
// is set up to only perform justworks pairing.
static gapBondCBs_t BLE_PowerBank_BondMgrCBs =
{
    (pfnPasscodeCB_t) BLE_PowerBank_passcodeCB, // Passcode callback
    BLE_PowerBank_pairStateCB                   // Pairing / Bonding state Callback
};

// Service callback function implementation
// LiPoBattService callback handler. The type LiPoBattServiceCBs_t is defined in LiPoBattService.h
static LiPoBattServiceCBs_t BLE_PowerBank_LiPoBattServiceCBs =
{
    BLE_PowerBank_LiPoBattServiceValueChangeCB // Characteristic value change callback handler
};


/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*
 *  ======== BLE_powerbankCallbackFxn ========
 *  Pin interrupt Callback function board buttons configured in the pinTable.
 */
void BLE_powerbankCallbackFxn(PIN_Handle handle, PIN_Id pinId)
{
    /* Debounce logic, only toggle if the button is still pushed (low) */
    CPUdelay(8000*50);

    switch (pinId)
    {
        case Board_PIN_BUTTON1:
        if (!PIN_getInputValue(pinId))
        {
            Util_restartClock(&periodicClock, BLE_PERIODIC_EVT_PERIOD);
        }
        break;

        case BATPAKMKII_BP_CHA_PIN:
        {
            // Check if BATPAKMKII CHA Pin is High or Low - Charging
            if(!PIN_getInputValue(BATPAKMKII_BP_CHA_PIN))
            {
                chargePinFlag = 1;
            }
            else
            {
                chargePinFlag = 0;
            }

            Event_post(syncEvent, SBP_CHA_DET_EVT);
        }
        break;

        default:
        /* Do nothing */
        break;
    }
}

#ifdef WATCHDOG_EN
void Watchdog__Callback(uintptr_t unused)
{
    HAL_SYSTEM_RESET();
}
#endif

static void BLE_PowerBank_LiPoBattServiceValueChangeCB(uint8_t paramID)
{
    switch (paramID)
    {
        case LIPOBATTSERVICE_LIPOBATTCRITCHAR:
            LiPoBattService_GetParameter( paramID, &battcrit );
        break;
        case LIPOBATTSERVICE_LIPOBATTHICHAR:
            LiPoBattService_GetParameter( paramID, &batthi );
        break;
        default:
        /* Do nothing */
        break;
    }
}

/*********************************************************************
 * @fn      BLE_PowerBank_createTask
 *
 * @brief   Task creation function for the Simple Peripheral.
 *
 * @param   None.
 *
 * @return  None.
 */
void BLE_PowerBank_createTask(void)
{
    Task_Params taskParams;

    // Configure task
    Task_Params_init(&taskParams);
    taskParams.stack = sbpTaskStack;
    taskParams.stackSize = SBP_TASK_STACK_SIZE;
    taskParams.priority = SBP_TASK_PRIORITY;

    Task_construct(&sbpTask, BLE_PowerBank_taskFxn, &taskParams, NULL);
}

/*********************************************************************
 * @fn      BLE_PowerBank_init
 *
 * @brief   Called during initialization and contains application
 *          specific initialization (ie. hardware initialization/setup,
 *          table initialization, power up notification, etc), and
 *          profile initialization/setup.
 *
 * @param   None.
 *
 * @return  None.
 */
static void BLE_PowerBank_init(void)
{
    uint8 snv_status = 0;
    /* Get the reason for reset */
    uint32_t rSrc = 0;

    /* Shut down external flash on LaunchPads. It is powered on by default
     * but can be shut down through SPI
     */
    Board_shutDownExtFlash();

    I2C_Init();

    dispHandle = Display_open(SBP_DISPLAY_TYPE, NULL);
    Display_clear(dispHandle);

    Display_print0(dispHandle, 0, 1, "BLE POWER BANK");

    Display_print0(dispHandle, 2, 0, "CHARGE: ");
    Display_print0(dispHandle, 3, 0, "VOLTAGE: ");
    Display_print0(dispHandle, 4, 0, "REM CAP: ");
    Display_print0(dispHandle, 5, 0, "CURRENT: ");
    Display_print0(dispHandle, 6, 0, "TEMP: ");

    blePowerBankPinHandle = PIN_open(&blePowerBankPinState, BLEPowerBankPinTable);

    if(!blePowerBankPinHandle)
    {
        /* Error initializing BLE Power Bank pins */
        while(1);
    }

    /* Setup callback for BLE Power Bank pins */
    if (PIN_registerIntCb(blePowerBankPinHandle, &BLE_powerbankCallbackFxn) != 0)
    {
        /* Error registering button callback function */
        while(1);
    }

    // Check if BATPAKMKII CHA Pin is High or Low - Charging
    if(!PIN_getInputValue(BATPAKMKII_BP_CHA_PIN))
    {
        Display_print0(dispHandle, 8, 0, "PB CHARGE: ON");
        PIN_setOutputValue(blePowerBankPinHandle, Board_MOS_TRIG, 0);
    }
    else
    {
        Display_print0(dispHandle, 8, 0, "PB CHARGE: OFF");
        PIN_setOutputValue(blePowerBankPinHandle, Board_MOS_TRIG, 1);
    }

    rSrc = SysCtrlResetSourceGet();

    if(rSrc != RSTSRC_WAKEUP_FROM_SHUTDOWN)
    {
        if (!BQ27441_initConfig())
        {
            while(1);
        }
    }

    // Read BQ27441 and print values at LCD
    BLE_PowerBank_ReadBQ27441();

    // ******************************************************************
    // N0 STACK API CALLS CAN OCCUR BEFORE THIS CALL TO ICall_registerApp
    // ******************************************************************
    // Register the current thread as an ICall dispatcher application
    // so that the application can send and receive messages.
    ICall_registerApp(&selfEntity, &syncEvent);

    // Create an RTOS queue for message from profile to be sent to app.
    appMsgQueue = Util_constructQueue(&appMsg);

    snv_status = osal_snv_read(SNV_ID_LIPOBATTHI, 2, &batthi);

    if(snv_status != SUCCESS)
    {
        //Write first time to initialize SNV ID
        osal_snv_write(SNV_ID_LIPOBATTHI, 2, LIPO_BATTHI);
        batthi = LIPO_BATTHI;
    }

    snv_status = osal_snv_read(SNV_ID_LIPOBATTCRIT, 2, &battcrit);

    if(snv_status != SUCCESS)
    {
        //Write first time to initialize SNV ID
        osal_snv_write(SNV_ID_LIPOBATTCRIT, 2, LIPO_BATTCRIT);
        battcrit = LIPO_BATTCRIT;
    }


#ifdef WATCHDOG_EN
    Watchdog_Init();
#endif
    // Create one-shot clocks for internal periodic events.
    Util_constructClock(&periodicClock, BLE_PowerBank_clockHandler,
                        BLE_PERIODIC_EVT_PERIOD, 0, true, SBP_PERIODIC_EVT);


    Util_constructClock(&bq27441readClock, BLE_PowerBank_clockHandler,
                        BQ27441_READ_EVT_PERIOD, 0, true, SBP_BQ27441_EVT);

    // Set GAP Parameters: After a connection was established, delay in seconds
    // before sending when GAPRole_SetParameter(GAPROLE_PARAM_UPDATE_ENABLE,...)
    // uses GAPROLE_LINK_PARAM_UPDATE_INITIATE_BOTH_PARAMS or
    // GAPROLE_LINK_PARAM_UPDATE_INITIATE_APP_PARAMS
    // For current defaults, this has no effect.
    GAP_SetParamValue(TGAP_CONN_PAUSE_PERIPHERAL, DEFAULT_CONN_PAUSE_PERIPHERAL);

    // Setup the Peripheral GAPRole Profile. For more information see the User's
    // Guide:
    // http://software-dl.ti.com/lprf/sdg-latest/html/
    {
        // Device starts advertising upon initialization of GAP
        uint8_t initialAdvertEnable = TRUE;

        // By setting this to zero, the device will go into the waiting state after
        // being discoverable for 30.72 second, and will not being advertising again
        // until re-enabled by the application
        uint16_t advertOffTime = 0;

        uint8_t enableUpdateRequest = DEFAULT_ENABLE_UPDATE_REQUEST;
        uint16_t desiredMinInterval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
        uint16_t desiredMaxInterval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
        uint16_t desiredSlaveLatency = DEFAULT_DESIRED_SLAVE_LATENCY;
        uint16_t desiredConnTimeout = DEFAULT_DESIRED_CONN_TIMEOUT;

        // Set the Peripheral GAPRole Parameters
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t),
                             &initialAdvertEnable);
        GAPRole_SetParameter(GAPROLE_ADVERT_OFF_TIME, sizeof(uint16_t),
                             &advertOffTime);

        GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData),
                             scanRspData);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);

        GAPRole_SetParameter(GAPROLE_PARAM_UPDATE_ENABLE, sizeof(uint8_t),
                             &enableUpdateRequest);
        GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16_t),
                             &desiredMinInterval);
        GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16_t),
                             &desiredMaxInterval);
        GAPRole_SetParameter(GAPROLE_SLAVE_LATENCY, sizeof(uint16_t),
                             &desiredSlaveLatency);
        GAPRole_SetParameter(GAPROLE_TIMEOUT_MULTIPLIER, sizeof(uint16_t),
                             &desiredConnTimeout);
    }

    // Set the Device Name characteristic in the GAP GATT Service
    // For more information, see the section in the User's Guide:
    // http://software-dl.ti.com/lprf/sdg-latest/html
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName);

    // Set GAP Parameters to set the advertising interval
    // For more information, see the GAP section of the User's Guide:
    // http://software-dl.ti.com/lprf/sdg-latest/html
    {
        // Use the same interval for general and limited advertising.
        // Note that only general advertising will occur based on the above configuration
        uint16_t advInt = DEFAULT_ADVERTISING_INTERVAL;

        GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MIN, advInt);
        GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MAX, advInt);
        GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, advInt);
        GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, advInt);
    }

    // Setup the GAP Bond Manager. For more information see the section in the
    // User's Guide:
    // http://software-dl.ti.com/lprf/sdg-latest/html/
    {
        // Don't send a pairing request after connecting; the peer device must
        // initiate pairing
        uint8_t pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
        // Use authenticated pairing: require passcode.
        uint8_t mitm = TRUE;
        // This device only has display capabilities. Therefore, it will display the
        // passcode during pairing. However, since the default passcode is being
        // used, there is no need to display anything.
        uint8_t ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
        // Request bonding (storing long-term keys for re-encryption upon subsequent
        // connections without repairing)
        uint8_t bonding = TRUE;

        GAPBondMgr_SetParameter(GAPBOND_PAIRING_MODE, sizeof(uint8_t), &pairMode);
        GAPBondMgr_SetParameter(GAPBOND_MITM_PROTECTION, sizeof(uint8_t), &mitm);
        GAPBondMgr_SetParameter(GAPBOND_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
        GAPBondMgr_SetParameter(GAPBOND_BONDING_ENABLED, sizeof(uint8_t), &bonding);
    }

    // Initialize GATT attributes
    GGS_AddService(GATT_ALL_SERVICES);           // GAP GATT Service
    GATTServApp_AddService(GATT_ALL_SERVICES);   // GATT Service
#ifdef DEV_INFO
    DevInfo_AddService();                        // Device Information Service
#endif

    LiPoBattService_AddService();
    LiPoBattService_RegisterAppCBs(&BLE_PowerBank_LiPoBattServiceCBs);

    // Initalization of characteristics in LiPoBattService that are readable.
    LiPoBattService_SetParameter(LIPOBATTSERVICE_LIPOBATTVALCHAR, LIPOBATTSERVICE_LIPOBATTVALCHAR_LEN, 0);
    LiPoBattService_SetParameter(LIPOBATTSERVICE_LIPOBATTNOTICHAR, LIPOBATTSERVICE_LIPOBATTNOTICHAR_LEN, &notval);
    LiPoBattService_SetParameter(LIPOBATTSERVICE_LIPOBATTCRITCHAR, LIPOBATTSERVICE_LIPOBATTCRITCHAR_LEN, &battcrit);
    LiPoBattService_SetParameter(LIPOBATTSERVICE_LIPOBATTHICHAR, LIPOBATTSERVICE_LIPOBATTHICHAR_LEN, &batthi);

    // Start the Device
    VOID GAPRole_StartDevice(&BLE_PowerBank_gapRoleCBs);

    // Start Bond Manager and register callback
    VOID GAPBondMgr_Register(&BLE_PowerBank_BondMgrCBs);

    // Register with GAP for HCI/Host messages. This is needed to receive HCI
    // events. For more information, see the section in the User's Guide:
    // http://software-dl.ti.com/lprf/sdg-latest/html
    GAP_RegisterForMsgs(selfEntity);

    // Register for GATT local events and ATT Responses pending for transmission
    GATT_RegisterForMsgs(selfEntity);

    //Set default values for Data Length Extension
    {
        //Set initial values to maximum, RX is set to max. by default(251 octets, 2120us)
        #define APP_SUGGESTED_PDU_SIZE 251 //default is 27 octets(TX)
        #define APP_SUGGESTED_TX_TIME 2120 //default is 328us(TX)

        //This API is documented in hci.h
        //See the LE Data Length Extension section in the BLE-Stack User's Guide for information on using this command:
        //http://software-dl.ti.com/lprf/sdg-latest/html/cc2640/index.html
        //HCI_LE_WriteSuggestedDefaultDataLenCmd(APP_SUGGESTED_PDU_SIZE, APP_SUGGESTED_TX_TIME);
    }

#if !defined (USE_LL_CONN_PARAM_UPDATE)
    // Get the currently set local supported LE features
    // The HCI will generate an HCI event that will get received in the main
    // loop
    HCI_LE_ReadLocalSupportedFeaturesCmd();
#endif // !defined (USE_LL_CONN_PARAM_UPDATE)


}

/*********************************************************************
 * @fn      BLE_PowerBank_taskFxn
 *
 * @brief   Application task entry point for the Simple Peripheral.
 *
 * @param   a0, a1 - not used.
 *
 * @return  None.
 */
static void BLE_PowerBank_taskFxn(UArg a0, UArg a1)
{
    // Initialize application
    BLE_PowerBank_init();

    // Application main loop
    for (;;)
    {
        uint32_t events;

        // Waits for an event to be posted associated with the calling thread.
        // Note that an event associated with a thread is posted when a
        // message is queued to the message receive queue of the thread
        events = Event_pend(syncEvent, Event_Id_NONE, SBP_ALL_EVENTS,
                            ICALL_TIMEOUT_FOREVER);
#ifdef WATCHDOG_EN
        Watchdog_clear(WatchdogHandle);
#endif
        if (events)
        {
            ICall_EntityID dest;
            ICall_ServiceEnum src;
            ICall_HciExtEvt *pMsg = NULL;

            // Fetch any available messages that might have been sent from the stack
            if (ICall_fetchServiceMsg(&src, &dest,
                                    (void **)&pMsg) == ICALL_ERRNO_SUCCESS)
            {
                uint8 safeToDealloc = TRUE;

                if ((src == ICALL_SERVICE_CLASS_BLE) && (dest == selfEntity))
                {
                    ICall_Stack_Event *pEvt = (ICall_Stack_Event *)pMsg;

                    // Check for BLE stack events first
                    if (pEvt->signature == 0xffff)
                    {
                        // The GATT server might have returned a blePending as it was trying
                        // to process an ATT Response. Now that we finished with this
                        // connection event, let's try sending any remaining ATT Responses
                        // on the next connection event.
                        if (pEvt->event_flag & SBP_HCI_CONN_EVT_END_EVT)
                        {
                            // Try to retransmit pending ATT Response (if any)
                            BLE_PowerBank_sendAttRsp();
                        }
                    }
                    else
                    {
                        // Process inter-task message
                        safeToDealloc = BLE_PowerBank_processStackMsg((ICall_Hdr *)pMsg);
                    }
                }

                if (pMsg && safeToDealloc)
                {
                    ICall_freeMsg(pMsg);
                }
            }

            // If RTOS queue is not empty, process app message.
            if (events & SBP_QUEUE_EVT)
            {
                while (!Queue_empty(appMsgQueue))
                {
                    sbpEvt_t *pMsg = (sbpEvt_t *)Util_dequeueMsg(appMsgQueue);
                    if (pMsg)
                    {
                        // Process message.
                        BLE_PowerBank_processAppMsg(pMsg);

                        // Free the space from the message.
                        ICall_free(pMsg);
                    }
                }
            }

            if (events & SBP_PERIODIC_EVT)
            {
                if ((GapProfileState == GAPROLE_CONNECTED) || (hidischarge_flag == 1))
                {
                    Util_restartClock(&periodicClock, BLE_PERIODIC_EVT_PERIOD);
                }
                else
                {
                    // Perform periodic application task
                    BLE_PowerBank_Shutdown();
                }

            }

            if (events & SBP_CHA_DET_EVT)
            {
                if(chargePinFlag == 1)
                {
                    Display_print0(dispHandle, 8, 0, "PB CHARGE: ON");
                    PIN_setOutputValue(blePowerBankPinHandle, Board_MOS_TRIG, 0);
                }
                else
                {
                    Display_print0(dispHandle, 8, 0, "PB CHARGE: OFF");
                    PIN_setOutputValue(blePowerBankPinHandle, Board_MOS_TRIG, 1);
                }
            }

            if (events & SBP_BQ27441_EVT)
            {
                BLE_PowerBank_ReadBQ27441();
                Util_restartClock(&bq27441readClock, BQ27441_READ_EVT_PERIOD);
            }
        }
    }
}

/*********************************************************************
 * @fn      BLE_PowerBank_processStackMsg
 *
 * @brief   Process an incoming stack message.
 *
 * @param   pMsg - message to process
 *
 * @return  TRUE if safe to deallocate incoming message, FALSE otherwise.
 */
static uint8_t BLE_PowerBank_processStackMsg(ICall_Hdr *pMsg)
{
    uint8_t safeToDealloc = TRUE;

    switch (pMsg->event)
    {
        case GATT_MSG_EVENT:
            // Process GATT message
            safeToDealloc = BLE_PowerBank_processGATTMsg((gattMsgEvent_t *)pMsg);
        break;

        case HCI_GAP_EVENT_EVENT:
        {
            // Process HCI message
            switch(pMsg->status)
            {
                case HCI_COMMAND_COMPLETE_EVENT_CODE:
                // Process HCI Command Complete Event
                {

#if !defined (USE_LL_CONN_PARAM_UPDATE)
                    // This code will disable the use of the LL_CONNECTION_PARAM_REQ
                    // control procedure (for connection parameter updates, the
                    // L2CAP Connection Parameter Update procedure will be used
                    // instead). To re-enable the LL_CONNECTION_PARAM_REQ control
                    // procedures, define the symbol USE_LL_CONN_PARAM_UPDATE
                    // The L2CAP Connection Parameter Update procedure is used to
                    // support a delta between the minimum and maximum connection
                    // intervals required by some iOS devices.

                    // Parse Command Complete Event for opcode and status
                    hciEvt_CmdComplete_t* command_complete = (hciEvt_CmdComplete_t*) pMsg;
                    uint8_t   pktStatus = command_complete->pReturnParam[0];

                    //find which command this command complete is for
                    switch (command_complete->cmdOpcode)
                    {

                        case HCI_LE_READ_LOCAL_SUPPORTED_FEATURES:
                        {
                            if (pktStatus == SUCCESS)
                            {
                                uint8_t featSet[8];

                                // Get current feature set from received event (bits 1-9
                                // of the returned data
                                memcpy( featSet, &command_complete->pReturnParam[1], 8);

                                // Clear bit 1 of byte 0 of feature set to disable LL
                                // Connection Parameter Updates
                                CLR_FEATURE_FLAG(featSet[0], LL_FEATURE_CONN_PARAMS_REQ);

                                // Update controller with modified features
                                HCI_EXT_SetLocalSupportedFeaturesCmd(featSet);
                            }
                        }
                        break;

                        default:
                            //do nothing
                        break;
                    }
#endif // !defined (USE_LL_CONN_PARAM_UPDATE)

                }
                break;

                case HCI_BLE_HARDWARE_ERROR_EVENT_CODE:
                    AssertHandler(HAL_ASSERT_CAUSE_HARDWARE_ERROR,0);
                break;

                default:
                    //do nothing
                break;
            }
        }
        break;

        default:
        // do nothing
        break;

    }

    return (safeToDealloc);
}

/*********************************************************************
 * @fn      BLE_PowerBank_processGATTMsg
 *
 * @brief   Process GATT messages and events.
 *
 * @return  TRUE if safe to deallocate incoming message, FALSE otherwise.
 */
static uint8_t BLE_PowerBank_processGATTMsg(gattMsgEvent_t *pMsg)
{
    // See if GATT server was unable to transmit an ATT response
    if (pMsg->hdr.status == blePending)
    {
        // No HCI buffer was available. Let's try to retransmit the response
        // on the next connection event.
        if (HCI_EXT_ConnEventNoticeCmd(pMsg->connHandle, selfEntity,
                                   SBP_HCI_CONN_EVT_END_EVT) == SUCCESS)
        {
            // First free any pending response
            BLE_PowerBank_freeAttRsp(FAILURE);

            // Hold on to the response message for retransmission
            pAttRsp = pMsg;

            // Don't free the response message yet
            return (FALSE);
        }
    }
    else if (pMsg->method == ATT_FLOW_CTRL_VIOLATED_EVENT)
    {
        // ATT request-response or indication-confirmation flow control is
        // violated. All subsequent ATT requests or indications will be dropped.
        // The app is informed in case it wants to drop the connection.

        // Display the opcode of the message that caused the violation.
#ifdef DEBUG_PRINT
        Display_print1(dispHandle, 5, 0, "FC Violated: %d", pMsg->msg.flowCtrlEvt.opcode);
#endif
    }
    else if (pMsg->method == ATT_MTU_UPDATED_EVENT)
    {
        // MTU size updated
#ifdef DEBUG_PRINT
        Display_print1(dispHandle, 5, 0, "MTU Size: %d", pMsg->msg.mtuEvt.MTU);
#endif
    }

    // Free message payload. Needed only for ATT Protocol messages
    GATT_bm_free(&pMsg->msg, pMsg->method);

    // It's safe to free the incoming message
    return (TRUE);
}

/*********************************************************************
 * @fn      BLE_PowerBank_sendAttRsp
 *
 * @brief   Send a pending ATT response message.
 *
 * @param   none
 *
 * @return  none
 */
static void BLE_PowerBank_sendAttRsp(void)
{
    // See if there's a pending ATT Response to be transmitted
    if (pAttRsp != NULL)
    {
        uint8_t status;

        // Increment retransmission count
        rspTxRetry++;

        // Try to retransmit ATT response till either we're successful or
        // the ATT Client times out (after 30s) and drops the connection.
        status = GATT_SendRsp(pAttRsp->connHandle, pAttRsp->method, &(pAttRsp->msg));
        if ((status != blePending) && (status != MSG_BUFFER_NOT_AVAIL))
        {
            // Disable connection event end notice
            HCI_EXT_ConnEventNoticeCmd(pAttRsp->connHandle, selfEntity, 0);

            // We're done with the response message
            BLE_PowerBank_freeAttRsp(status);
        }
        else
        {
            // Continue retrying
#ifdef DEBUG_PRINT
            //Display_print1(dispHandle, 5, 0, "Rsp send retry: %d", rspTxRetry);
#endif
        }
    }
}

/*********************************************************************
 * @fn      BLE_PowerBank_freeAttRsp
 *
 * @brief   Free ATT response message.
 *
 * @param   status - response transmit status
 *
 * @return  none
 */
static void BLE_PowerBank_freeAttRsp(uint8_t status)
{
    // See if there's a pending ATT response message
    if (pAttRsp != NULL)
    {
        // See if the response was sent out successfully
        if (status == SUCCESS)
        {
#ifdef DEBUG_PRINT
            Display_print1(dispHandle, 5, 0, "Rsp sent retry: %d", rspTxRetry);
#endif
        }
        else
        {
            // Free response payload
            GATT_bm_free(&pAttRsp->msg, pAttRsp->method);
#ifdef DEBUG_PRINT
            Display_print1(dispHandle, 5, 0, "Rsp retry failed: %d", rspTxRetry);
#endif
        }

        // Free response message
        ICall_freeMsg(pAttRsp);

        // Reset our globals
        pAttRsp = NULL;
        rspTxRetry = 0;
    }
}

/*********************************************************************
 * @fn      BLE_PowerBank_processAppMsg
 *
 * @brief   Process an incoming callback from a profile.
 *
 * @param   pMsg - message to process
 *
 * @return  None.
 */
static void BLE_PowerBank_processAppMsg(sbpEvt_t *pMsg)
{
    switch (pMsg->hdr.event)
    {
        case SBP_STATE_CHANGE_EVT:
        {
            BLE_PowerBank_processStateChangeEvt((gaprole_States_t)pMsg->
                                                hdr.state);
            break;
        }

        case SBP_CHAR_CHANGE_EVT:
        {
            BLE_PowerBank_processCharValueChangeEvt(pMsg->hdr.state);
            break;
        }

        // Pairing event
        case SBP_PAIRING_STATE_EVT:
        {
            BLE_PowerBank_processPairState(pMsg->hdr.state, *pMsg->pData);
            ICall_free(pMsg->pData);
            break;
        }

        // Passcode event
        case SBP_PASSCODE_NEEDED_EVT:
        {
            BLE_PowerBank_processPasscode(*pMsg->pData);

            ICall_free(pMsg->pData);
            break;
        }

        default:
            // Do nothing.
        break;
    }
}

/*********************************************************************
 * @fn      BLE_PowerBank_stateChangeCB
 *
 * @brief   Callback from GAP Role indicating a role state change.
 *
 * @param   newState - new state
 *
 * @return  None.
 */
static void BLE_PowerBank_stateChangeCB(gaprole_States_t newState)
{
    BLE_PowerBank_enqueueMsg(SBP_STATE_CHANGE_EVT, newState, 0);
}

/*********************************************************************
 * @fn      BLE_PowerBank_processStateChangeEvt
 *
 * @brief   Process a pending GAP Role state change event.
 *
 * @param   newState - new state
 *
 * @return  None.
 */
static void BLE_PowerBank_processStateChangeEvt(gaprole_States_t newState)
{

    GapProfileState = newState;

    switch ( newState )
    {
        case GAPROLE_STARTED:
        {
#ifdef DEV_INFO
            uint8_t ownAddress[B_ADDR_LEN];
            uint8_t systemId[DEVINFO_SYSTEM_ID_LEN];

            GAPRole_GetParameter(GAPROLE_BD_ADDR, ownAddress);

            // use 6 bytes of device address for 8 bytes of system ID value
            systemId[0] = ownAddress[0];
            systemId[1] = ownAddress[1];
            systemId[2] = ownAddress[2];

            // set middle bytes to zero
            systemId[4] = 0x00;
            systemId[3] = 0x00;

            // shift three bytes up
            systemId[7] = ownAddress[5];
            systemId[6] = ownAddress[4];
            systemId[5] = ownAddress[3];


            DevInfo_SetParameter(DEVINFO_SYSTEM_ID, DEVINFO_SYSTEM_ID_LEN, systemId);
#endif

#ifdef DEBUG_PRINT
            // Display device address
            Display_print0(dispHandle, 1, 0, Util_convertBdAddr2Str(ownAddress));

            Display_print0(dispHandle, 7, 0, "BLE Status: Initialized");
#endif

        }
        break;

        case GAPROLE_ADVERTISING:
#ifdef DEBUG_PRINT
            Display_print0(dispHandle, 2, 0, "Advertising");
#endif
        break;

        case GAPROLE_CONNECTED:
        {
            linkDBInfo_t linkInfo;
            uint8_t numActive = 0;

            Util_restartClock(&periodicClock, BLE_PERIODIC_EVT_PERIOD);

            numActive = linkDB_NumActive();

            // Use numActive to determine the connection handle of the last
            // connection
            if ( linkDB_GetInfo( numActive - 1, &linkInfo ) == SUCCESS )
            {

#ifdef DEBUG_PRINT
                Display_print1(dispHandle, 2, 0, "Num Conns: %d", (uint16_t)numActive);
                Display_print0(dispHandle, 3, 0, Util_convertBdAddr2Str(linkInfo.addr));
#endif
            }
            else
            {
                uint8_t peerAddress[B_ADDR_LEN];

                GAPRole_GetParameter(GAPROLE_CONN_BD_ADDR, peerAddress);

#ifdef DEBUG_PRINT
                Display_print0(dispHandle, 2, 0, "Connected");
                Display_print0(dispHandle, 3, 0, Util_convertBdAddr2Str(peerAddress));
#endif
            }

            Display_print0(dispHandle, 9, 0, "BLE Connected");
        }
        break;

        case GAPROLE_CONNECTED_ADV:
#ifdef DEBUG_PRINT
            Display_print0(dispHandle, 2, 0, "Connected Advertising");
#endif
        break;

        case GAPROLE_WAITING:
            Util_restartClock(&periodicClock, BLE_PERIODIC_EVT_PERIOD);
            BLE_PowerBank_freeAttRsp(bleNotConnected);
            Display_print0(dispHandle, 9, 0, "BLE Disconnected");
#ifdef DEBUG_PRINT
            Display_print0(dispHandle, 2, 0, "Disconnected");


            // Clear remaining lines
            Display_clearLines(dispHandle, 3, 5);
#endif
        break;

        case GAPROLE_WAITING_AFTER_TIMEOUT:
            Util_restartClock(&periodicClock, BLE_PERIODIC_EVT_PERIOD);
            BLE_PowerBank_freeAttRsp(bleNotConnected);
            Display_print0(dispHandle, 8, 0, "BLE Time Out");
#ifdef DEBUG_PRINT
            Display_print0(dispHandle, 2, 0, "Timed Out");

            // Clear remaining lines
            Display_clearLines(dispHandle, 3, 5);
#endif
        break;

        case GAPROLE_ERROR:
#ifdef DEBUG_PRINT
            Display_print0(dispHandle, 2, 0, "Error");
#endif
        break;

        default:
#ifdef DEBUG_PRINT
            Display_clearLine(dispHandle, 2);
#endif
        break;
    }
}


/*********************************************************************
 * @fn      BLE_PowerBank_processCharValueChangeEvt
 *
 * @brief   Process a pending Simple Profile characteristic value change
 *          event.
 *
 * @param   paramID - parameter ID of the value that was changed.
 *
 * @return  None.
 */
static void BLE_PowerBank_processCharValueChangeEvt(uint8_t paramID)
{
    switch(paramID)
    {
        default:
        // should not reach here!
        break;
    }
}

/*********************************************************************
 * @fn      BLE_PowerBank_Shutdown
 *
 * @brief   Shutdown BLE Power Bank.
 *
 * @param   None.
 *
 * @return  None.
 */
static void BLE_PowerBank_Shutdown(void)
{
    uint8 snv_status;

    Display_clear(dispHandle);

    Display_print0(dispHandle, 5, 4, "SHUTDOWN");
    Display_print0(dispHandle, 6, 6, "MODE");

    CPUdelay(40000*50);
    CPUdelay(40000*50);
    CPUdelay(40000*50);

    Display_clear(dispHandle);
    Display_close(dispHandle);

    snv_status = osal_snv_write(SNV_ID_LIPOBATTHI, 2, &batthi);

    if(snv_status != SUCCESS)
    {
        while(1);
    }

    snv_status = osal_snv_write(SNV_ID_LIPOBATTCRIT, 2, &battcrit);

    if(snv_status != SUCCESS)
    {
        while(1);
    }

    PIN_setOutputValue(blePowerBankPinHandle, Board_MOS_TRIG, 0);

    /* Configure DIO for wake up from shutdown */
    PINCC26XX_setWakeup(ButtonTableWakeUp);

    /* Go to shutdown */
    Power_shutdown(0, 0);

    /* Should never get here, since shutdown will reset. */
    while(1);
}

static void BLE_PowerBank_ReadBQ27441(void)
{

    short hidischarge = -50;

    /* Read State Of Charge */
    if(!BQ27441_read16(STATE_OF_CHARGE, &bq27441charge))
    {
        Display_printf(dispHandle, 2, 0, "CHARGE: Err");
    }
    else
    {
        Display_printf(dispHandle, 2, 0, "CHARGE: %d%%", (unsigned short)bq27441charge);
        LiPoBattService_SetParameter(LIPOBATTSERVICE_LIPOBATTVALCHAR, LIPOBATTSERVICE_LIPOBATTVALCHAR_LEN, &bq27441charge);
    }

    /* Read Voltage */
    if(!BQ27441_read16(VOLTAGE, &bq27441voltage))
    {
        Display_printf(dispHandle, 3, 0, "VOLTAGE: Err");
    }
    else
    {
        Display_printf(dispHandle, 3, 0, "VOLTAGE: %dmV", bq27441voltage);
    }

    /* Read Remaining Capacity */
    if(!BQ27441_read16(REMAINING_CAPACITY, &bq27441remcap))
    {
        Display_printf(dispHandle, 4, 0, "REM CAP: Err");
    }
    else
    {
        Display_printf(dispHandle, 4, 0, "REM CAP: %dmAh", bq27441remcap);
    }

    /* Read Average Current */
    if(!BQ27441_read16(AVERAGE_CURRENT, &bq27441current))
    {
        Display_printf(dispHandle, 5, 0, "CURRENT: Err");
    }
    else
    {
        Display_printf(dispHandle, 5, 0, "CURRENT: %dmA", bq27441current);

        if (bq27441current > 0)
        {
            if ((batthi == bq27441charge) && (notval == 0))
            {
                notval = 2;
                LiPoBattService_SetParameter(LIPOBATTSERVICE_LIPOBATTNOTICHAR, LIPOBATTSERVICE_LIPOBATTNOTICHAR_LEN, &notval);
            }

            if (batthi > bq27441charge)
            {
                notval = 0;
            }
        }
        else
        {
            if(bq27441current < hidischarge)
            {
                // HIGH discharging
                hidischarge_flag = 1;
            }
            else
            {
                // NORMAL discharging
                hidischarge_flag = 0;
            }

            if ((battcrit == bq27441charge) && (notval == 0))
            {
                notval = 1;
                LiPoBattService_SetParameter(LIPOBATTSERVICE_LIPOBATTNOTICHAR, LIPOBATTSERVICE_LIPOBATTNOTICHAR_LEN, &notval);
            }

            if (battcrit < bq27441charge)
            {
                notval = 0;
            }
        }
    }

    /* Read Temperature */
    if(!BQ27441_read16(TEMPERATURE, &bq27441temp))
    {
        Display_printf(dispHandle, 6, 0, "TEMP: Err");
    }
    else
    {
        Display_printf(dispHandle, 6, 0, "TEMP:: %dC", bq27441temp/10 - 273);
    }


}

/*********************************************************************
 * @fn      BLE_PowerBank_pairStateCB
 *
 * @brief   Pairing state callback.
 *
 * @return  none
 */
static void BLE_PowerBank_pairStateCB(uint16_t connHandle, uint8_t state,
                                            uint8_t status)
{
    uint8_t *pData;

    // Allocate space for the event data.
    if ((pData = ICall_malloc(sizeof(uint8_t))))
    {
        *pData = status;

        // Queue the event.
        BLE_PowerBank_enqueueMsg(SBP_PAIRING_STATE_EVT, state, pData);
    }
}

/*********************************************************************
 * @fn      BLE_PowerBank_processPairState
 *
 * @brief   Process the new paring state.
 *
 * @return  none
 */
static void BLE_PowerBank_processPairState(uint8_t state, uint8_t status)
{
    if (state == GAPBOND_PAIRING_STATE_STARTED)
    {
#ifdef DEBUG_PRINT
        Display_print0(dispHandle, 2, 0, "Pairing started");
#endif
    }
    else if (state == GAPBOND_PAIRING_STATE_COMPLETE)
    {
        if (status == SUCCESS)
        {
#ifdef DEBUG_PRINT
            Display_print0(dispHandle, 2, 0, "Pairing success");
#endif
        }
        else
        {
#ifdef DEBUG_PRINT
            Display_print1(dispHandle, 2, 0, "Pairing fail: %d", status);
#endif
        }
    }
    else if (state == GAPBOND_PAIRING_STATE_BONDED)
    {
        if (status == SUCCESS)
        {
#ifdef DEBUG_PRINT
            Display_print0(dispHandle, 2, 0, "Bonding success");
#endif
        }
    }
    else if (state == GAPBOND_PAIRING_STATE_BOND_SAVED)
    {
        if (status == SUCCESS)
        {
#ifdef DEBUG_PRINT
            Display_print0(dispHandle, 2, 0, "Bond save success");
#endif
        }
        else
        {
#ifdef DEBUG_PRINT
            Display_print1(dispHandle, 2, 0, "Bond save failed: %d", status);
#endif
        }
    }
}

/*********************************************************************
 * @fn      BLE_PowerBank_passcodeCB
 *
 * @brief   Passcode callback.
 *
 * @return  none
 */
static void BLE_PowerBank_passcodeCB(uint8_t *deviceAddr, uint16_t connHandle,
                                           uint8_t uiInputs, uint8_t uiOutputs)
{
    uint8_t *pData;

    // Allocate space for the passcode event.
    if ((pData = ICall_malloc(sizeof(uint8_t))))
    {
        *pData = uiOutputs;

        // Enqueue the event.
        BLE_PowerBank_enqueueMsg(SBP_PASSCODE_NEEDED_EVT, 0, pData);
    }
}

/*********************************************************************
 * @fn      BLE_PowerBank_processPasscode
 *
 * @brief   Process the Passcode request.
 *
 * @return  none
 */
static void BLE_PowerBank_processPasscode(uint8_t uiOutputs)
{
    // This app uses a default passcode. A real-life scenario would handle all
    // pairing scenarios and likely generate this randomly.
    uint32_t passcode = B_APP_DEFAULT_PASSCODE;

    // Display passcode to user
    if (uiOutputs != 0)
    {
#ifdef DEBUG_PRINT
        Display_print1(dispHandle, 4, 0, "Passcode: %d", passcode);
#endif
    }

    uint16_t connectionHandle;
    GAPRole_GetParameter(GAPROLE_CONNHANDLE, &connectionHandle);

    // Send passcode response
    GAPBondMgr_PasscodeRsp(connectionHandle, SUCCESS, passcode);
}

/*********************************************************************
 * @fn      BLE_PowerBank_clockHandler
 *
 * @brief   Handler function for clock timeouts.
 *
 * @param   arg - event type
 *
 * @return  None.
 */
static void BLE_PowerBank_clockHandler(UArg arg)
{
  // Wake up the application.
  Event_post(syncEvent, arg);
}

/*********************************************************************
 * @fn      BLE_PowerBank_enqueueMsg
 *
 * @brief   Creates a message and puts the message in RTOS queue.
 *
 * @param   event - message event.
 * @param   state - message state.
 * @param   pData - message data pointer.
 *
 * @return  TRUE or FALSE
 */
static uint8_t BLE_PowerBank_enqueueMsg(uint8_t event, uint8_t state,
                                           uint8_t *pData)
{
  sbpEvt_t *pMsg = ICall_malloc(sizeof(sbpEvt_t));

  // Create dynamic pointer to message.
  if (pMsg)
  {
    pMsg->hdr.event = event;
    pMsg->hdr.state = state;
    pMsg->pData = pData;

    // Enqueue the message.
    return Util_enqueueMsg(appMsgQueue, syncEvent, (uint8_t *)pMsg);
  }

  return FALSE;
}

#ifdef WATCHDOG_EN
static void Watchdog_Init(void)
{
    Watchdog_Params params;

    Watchdog_init();

    Watchdog_Params_init(&params);
    params.callbackFxn = (Watchdog_Callback)Watchdog__Callback;
    params.resetMode = Watchdog_RESET_ON;
    WatchdogHandle = Watchdog_open(Board_WATCHDOG0, &params);

    if (WatchdogHandle == NULL) {
        /* Error opening Watchdog */
        while (1);
    }
}
#endif
/*********************************************************************
*********************************************************************/
