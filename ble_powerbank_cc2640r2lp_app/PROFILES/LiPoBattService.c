/**********************************************************************************************
 * Filename:       LiPoBattService.c
 *
 * Description:    This file contains the implementation of the service.
 *
 * Copyright (c) 2015-2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *************************************************************************************************/


/*********************************************************************
 * INCLUDES
 */
#include <string.h>

/* This Header file contains all BLE API and icall structure definition */
#include "icall_ble_api.h"

#include <icall.h>

#include "LiPoBattService.h"

/*********************************************************************
 * MACROS
 */
/**
 * GATT Characteristic Descriptions
 */
#define GATT_DESC_LENGTH_UUID            0x3111 // Used with Unit percent
/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
* GLOBAL VARIABLES
*/

// LiPoBattService Service UUID
CONST uint8_t LiPoBattServiceUUID[ATT_UUID_SIZE] =
{
  TI_BASE_UUID_128(LIPOBATTSERVICE_SERV_UUID)
};

// LipoBattValChar UUID
CONST uint8_t LiPoBattService_LipoBattValCharUUID[ATT_UUID_SIZE] =
{
  TI_BASE_UUID_128(LIPOBATTSERVICE_LIPOBATTVALCHAR_UUID)
};
// LiPoBattNotiChar UUID
CONST uint8_t LiPoBattService_LiPoBattNotiCharUUID[ATT_UUID_SIZE] =
{
  TI_BASE_UUID_128(LIPOBATTSERVICE_LIPOBATTNOTICHAR_UUID)
};
// LiPoBattCritChar UUID
CONST uint8_t LiPoBattService_LiPoBattCritCharUUID[ATT_UUID_SIZE] =
{
  TI_BASE_UUID_128(LIPOBATTSERVICE_LIPOBATTCRITCHAR_UUID)
};
// LiPoBattHiChar UUID
CONST uint8_t LiPoBattService_LiPoBattHiCharUUID[ATT_UUID_SIZE] =
{
  TI_BASE_UUID_128(LIPOBATTSERVICE_LIPOBATTHICHAR_UUID)
};

/*********************************************************************
 * LOCAL VARIABLES
 */

static LiPoBattServiceCBs_t *pAppCBs = NULL;

/*********************************************************************
* Profile Attributes - variables
*/

// Service declaration
static CONST gattAttrType_t LiPoBattServiceDecl = { ATT_UUID_SIZE, LiPoBattServiceUUID };

// Characteristic "LipoBattValChar" Properties (for declaration)
static uint8_t LiPoBattService_LipoBattValCharProps = GATT_PROP_READ | GATT_PROP_NOTIFY;

// Characteristic "LipoBattValChar" Value variable
static uint8_t LiPoBattService_LipoBattValCharVal[LIPOBATTSERVICE_LIPOBATTVALCHAR_LEN] = {0};

// Characteristic "LipoBattValChar" CCCD
static gattCharCfg_t *LiPoBattService_LipoBattValCharConfig;

// LipoBattValChar Characteristic User Description
static uint8 LipoBattValDesc[14] = "Battery Level";

// Characteristic "LiPoBattNotiChar" Properties (for declaration)
static uint8_t LiPoBattService_LiPoBattNotiCharProps = GATT_PROP_READ | GATT_PROP_NOTIFY;

// Characteristic "LiPoBattNotiChar" Value variable
static uint8_t LiPoBattService_LiPoBattNotiCharVal[LIPOBATTSERVICE_LIPOBATTNOTICHAR_LEN] = {0};

// Characteristic "LiPoBattNotiChar" CCCD
static gattCharCfg_t *LiPoBattService_LiPoBattNotiCharConfig;

// LipoNotiChar Characteristic User Description
static uint8 LipoNotiDesc[18] = "Batt Crit/Hi Noti";

// Characteristic "LiPoBattCritChar" Properties (for declaration)
static uint8_t LiPoBattService_LiPoBattCritCharProps = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic "LiPoBattCritChar" Value variable
static uint8_t LiPoBattService_LiPoBattCritCharVal[LIPOBATTSERVICE_LIPOBATTCRITCHAR_LEN] = {0};

// LipoBattCrit Characteristic User Description
static uint8 LipoBattCritDesc[16] = "Batt Crit Level";

// Characteristic "LiPoBattHiChar" Properties (for declaration)
static uint8_t LiPoBattService_LiPoBattHiCharProps = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic "LiPoBattHiChar" Value variable
static uint8_t LiPoBattService_LiPoBattHiCharVal[LIPOBATTSERVICE_LIPOBATTHICHAR_LEN] = {0};

// LipoBattHi Characteristic User Description
static uint8 LipoBattHiDesc[16] = "Batt High Level";

/*********************************************************************
* Profile Attributes - Table
*/

static gattAttribute_t LiPoBattServiceAttrTbl[] =
{
    // LiPoBattService Service Declaration
    {
        { ATT_BT_UUID_SIZE, primaryServiceUUID },
        GATT_PERMIT_READ,
        0,
        (uint8_t *)&LiPoBattServiceDecl
    },
    // LipoBattValChar Characteristic Declaration
    {
        { ATT_BT_UUID_SIZE, characterUUID },
        GATT_PERMIT_READ,
        0,
        &LiPoBattService_LipoBattValCharProps
    },
    // LipoBattValChar Characteristic Value
    {
        { ATT_UUID_SIZE, LiPoBattService_LipoBattValCharUUID },
        GATT_PERMIT_READ,
        0,
        LiPoBattService_LipoBattValCharVal
    },
    // LipoBattValChar CCCD
    {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)&LiPoBattService_LipoBattValCharConfig
    },
    // Characteristic User Description
    {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        LipoBattValDesc
    },


    // LiPoBattNotiChar Characteristic Declaration
    {
        { ATT_BT_UUID_SIZE, characterUUID },
        GATT_PERMIT_READ,
        0,
        &LiPoBattService_LiPoBattNotiCharProps
    },
    // LiPoBattNotiChar Characteristic Value
    {
        { ATT_UUID_SIZE, LiPoBattService_LiPoBattNotiCharUUID },
        GATT_PERMIT_READ,
        0,
        LiPoBattService_LiPoBattNotiCharVal
    },
    // LiPoBattNotiChar CCCD
    {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)&LiPoBattService_LiPoBattNotiCharConfig
    },
    // Characteristic User Description
    {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        LipoNotiDesc
    },

    // LiPoBattCritChar Characteristic Declaration
    {
        { ATT_BT_UUID_SIZE, characterUUID },
        GATT_PERMIT_READ,
        0,
        &LiPoBattService_LiPoBattCritCharProps
    },
    // LiPoBattCritChar Characteristic Value
    {
        { ATT_UUID_SIZE, LiPoBattService_LiPoBattCritCharUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        LiPoBattService_LiPoBattCritCharVal
    },
    // Characteristic User Description
    {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        LipoBattCritDesc
    },

    // LiPoBattHiChar Characteristic Declaration
    {
        { ATT_BT_UUID_SIZE, characterUUID },
        GATT_PERMIT_READ,
        0,
        &LiPoBattService_LiPoBattHiCharProps
    },
    // LiPoBattHiChar Characteristic Value
    {
        { ATT_UUID_SIZE, LiPoBattService_LiPoBattHiCharUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        LiPoBattService_LiPoBattHiCharVal
    },
    // Characteristic User Description
    {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        LipoBattHiDesc
    },

};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t LiPoBattService_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                           uint8 *pValue, uint16 *pLen, uint16 offset,
                                           uint16 maxLen, uint8 method );
static bStatus_t LiPoBattService_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                            uint8 *pValue, uint16 len, uint16 offset,
                                            uint8 method );

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Simple Profile Service Callbacks
CONST gattServiceCBs_t LiPoBattServiceCBs =
{
  LiPoBattService_ReadAttrCB,  // Read callback function pointer
  LiPoBattService_WriteAttrCB, // Write callback function pointer
  NULL                       // Authorization callback function pointer
};

/*********************************************************************
* PUBLIC FUNCTIONS
*/

/*
 * LiPoBattService_AddService- Initializes the LiPoBattService service by registering
 *          GATT attributes with the GATT server.
 *
 */
bStatus_t LiPoBattService_AddService( void )
{
  uint8_t status;

  // Allocate Client Characteristic Configuration table
  LiPoBattService_LipoBattValCharConfig = (gattCharCfg_t *)ICall_malloc( sizeof(gattCharCfg_t) * linkDBNumConns );
  if ( LiPoBattService_LipoBattValCharConfig == NULL )
  {
    return ( bleMemAllocError );
  }

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, LiPoBattService_LipoBattValCharConfig );
  // Allocate Client Characteristic Configuration table
  LiPoBattService_LiPoBattNotiCharConfig = (gattCharCfg_t *)ICall_malloc( sizeof(gattCharCfg_t) * linkDBNumConns );
  if ( LiPoBattService_LiPoBattNotiCharConfig == NULL )
  {
    return ( bleMemAllocError );
  }

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, LiPoBattService_LiPoBattNotiCharConfig );
  // Register GATT attribute list and CBs with GATT Server App
  status = GATTServApp_RegisterService( LiPoBattServiceAttrTbl,
                                        GATT_NUM_ATTRS( LiPoBattServiceAttrTbl ),
                                        GATT_MAX_ENCRYPT_KEY_SIZE,
                                        &LiPoBattServiceCBs );

  return ( status );
}

/*
 * LiPoBattService_RegisterAppCBs - Registers the application callback function.
 *                    Only call this function once.
 *
 *    appCallbacks - pointer to application callbacks.
 */
bStatus_t LiPoBattService_RegisterAppCBs( LiPoBattServiceCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    pAppCBs = appCallbacks;

    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}

/*
 * LiPoBattService_SetParameter - Set a LiPoBattService parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
bStatus_t LiPoBattService_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case LIPOBATTSERVICE_LIPOBATTVALCHAR:
      if ( len == LIPOBATTSERVICE_LIPOBATTVALCHAR_LEN )
      {
        memcpy(LiPoBattService_LipoBattValCharVal, value, len);

        // Try to send notification.
        GATTServApp_ProcessCharCfg( LiPoBattService_LipoBattValCharConfig, (uint8_t *)&LiPoBattService_LipoBattValCharVal, FALSE,
                                    LiPoBattServiceAttrTbl, GATT_NUM_ATTRS( LiPoBattServiceAttrTbl ),
                                    INVALID_TASK_ID,  LiPoBattService_ReadAttrCB);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case LIPOBATTSERVICE_LIPOBATTNOTICHAR:
      if ( len == LIPOBATTSERVICE_LIPOBATTNOTICHAR_LEN )
      {
        memcpy(LiPoBattService_LiPoBattNotiCharVal, value, len);

        // Try to send notification.
        GATTServApp_ProcessCharCfg( LiPoBattService_LiPoBattNotiCharConfig, (uint8_t *)&LiPoBattService_LiPoBattNotiCharVal, FALSE,
                                    LiPoBattServiceAttrTbl, GATT_NUM_ATTRS( LiPoBattServiceAttrTbl ),
                                    INVALID_TASK_ID,  LiPoBattService_ReadAttrCB);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case LIPOBATTSERVICE_LIPOBATTCRITCHAR:
      if ( len == LIPOBATTSERVICE_LIPOBATTCRITCHAR_LEN )
      {
        memcpy(LiPoBattService_LiPoBattCritCharVal, value, len);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case LIPOBATTSERVICE_LIPOBATTHICHAR:
      if ( len == LIPOBATTSERVICE_LIPOBATTHICHAR_LEN )
      {
        memcpy(LiPoBattService_LiPoBattHiCharVal, value, len);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  return ret;
}


/*
 * LiPoBattService_GetParameter - Get a LiPoBattService parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
bStatus_t LiPoBattService_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case LIPOBATTSERVICE_LIPOBATTCRITCHAR:
      memcpy(value, LiPoBattService_LiPoBattCritCharVal, LIPOBATTSERVICE_LIPOBATTCRITCHAR_LEN);
      break;

    case LIPOBATTSERVICE_LIPOBATTHICHAR:
      memcpy(value, LiPoBattService_LiPoBattHiCharVal, LIPOBATTSERVICE_LIPOBATTHICHAR_LEN);
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  return ret;
}


/*********************************************************************
 * @fn          LiPoBattService_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 * @param       method - type of read message
 *
 * @return      SUCCESS, blePending or Failure
 */
static bStatus_t LiPoBattService_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                       uint8 *pValue, uint16 *pLen, uint16 offset,
                                       uint16 maxLen, uint8 method )
{
  bStatus_t status = SUCCESS;

  // See if request is regarding the LipoBattValChar Characteristic Value
if ( ! memcmp(pAttr->type.uuid, LiPoBattService_LipoBattValCharUUID, pAttr->type.len) )
  {
    if ( offset > LIPOBATTSERVICE_LIPOBATTVALCHAR_LEN )  // Prevent malicious ATT ReadBlob offsets.
    {
      status = ATT_ERR_INVALID_OFFSET;
    }
    else
    {
      *pLen = MIN(maxLen, LIPOBATTSERVICE_LIPOBATTVALCHAR_LEN - offset);  // Transmit as much as possible
      memcpy(pValue, pAttr->pValue + offset, *pLen);
    }
  }
  // See if request is regarding the LiPoBattNotiChar Characteristic Value
else if ( ! memcmp(pAttr->type.uuid, LiPoBattService_LiPoBattNotiCharUUID, pAttr->type.len) )
  {
    if ( offset > LIPOBATTSERVICE_LIPOBATTNOTICHAR_LEN )  // Prevent malicious ATT ReadBlob offsets.
    {
      status = ATT_ERR_INVALID_OFFSET;
    }
    else
    {
      *pLen = MIN(maxLen, LIPOBATTSERVICE_LIPOBATTNOTICHAR_LEN - offset);  // Transmit as much as possible
      memcpy(pValue, pAttr->pValue + offset, *pLen);
    }
  }
  // See if request is regarding the LiPoBattCritChar Characteristic Value
else if ( ! memcmp(pAttr->type.uuid, LiPoBattService_LiPoBattCritCharUUID, pAttr->type.len) )
  {
    if ( offset > LIPOBATTSERVICE_LIPOBATTCRITCHAR_LEN )  // Prevent malicious ATT ReadBlob offsets.
    {
      status = ATT_ERR_INVALID_OFFSET;
    }
    else
    {
      *pLen = MIN(maxLen, LIPOBATTSERVICE_LIPOBATTCRITCHAR_LEN - offset);  // Transmit as much as possible
      memcpy(pValue, pAttr->pValue + offset, *pLen);
    }
  }
  // See if request is regarding the LiPoBattHiChar Characteristic Value
else if ( ! memcmp(pAttr->type.uuid, LiPoBattService_LiPoBattHiCharUUID, pAttr->type.len) )
  {
    if ( offset > LIPOBATTSERVICE_LIPOBATTHICHAR_LEN )  // Prevent malicious ATT ReadBlob offsets.
    {
      status = ATT_ERR_INVALID_OFFSET;
    }
    else
    {
      *pLen = MIN(maxLen, LIPOBATTSERVICE_LIPOBATTHICHAR_LEN - offset);  // Transmit as much as possible
      memcpy(pValue, pAttr->pValue + offset, *pLen);
    }
  }
  else
  {
    // If we get here, that means you've forgotten to add an if clause for a
    // characteristic value attribute in the attribute table that has READ permissions.
    *pLen = 0;
    status = ATT_ERR_ATTR_NOT_FOUND;
  }

  return status;
}


/*********************************************************************
 * @fn      LiPoBattService_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   method - type of write message
 *
 * @return  SUCCESS, blePending or Failure
 */
static bStatus_t LiPoBattService_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                        uint8 *pValue, uint16 len, uint16 offset,
                                        uint8 method )
{
  bStatus_t status  = SUCCESS;
  uint8_t   paramID = 0xFF;

  // See if request is regarding a Client Characterisic Configuration
  if ( ! memcmp(pAttr->type.uuid, clientCharCfgUUID, pAttr->type.len) )
  {
    // Allow only notifications.
    status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                             offset, GATT_CLIENT_CFG_NOTIFY);
  }
  // See if request is regarding the LiPoBattCritChar Characteristic Value
  else if ( ! memcmp(pAttr->type.uuid, LiPoBattService_LiPoBattCritCharUUID, pAttr->type.len) )
  {
    if ( offset + len > LIPOBATTSERVICE_LIPOBATTCRITCHAR_LEN )
    {
      status = ATT_ERR_INVALID_OFFSET;
    }
    else
    {
      // Copy pValue into the variable we point to from the attribute table.
      memcpy(pAttr->pValue + offset, pValue, len);

      // Only notify application if entire expected value is written
      if ( offset + len == LIPOBATTSERVICE_LIPOBATTCRITCHAR_LEN)
        paramID = LIPOBATTSERVICE_LIPOBATTCRITCHAR;
    }
  }
  // See if request is regarding the LiPoBattHiChar Characteristic Value
  else if ( ! memcmp(pAttr->type.uuid, LiPoBattService_LiPoBattHiCharUUID, pAttr->type.len) )
  {
    if ( offset + len > LIPOBATTSERVICE_LIPOBATTHICHAR_LEN )
    {
      status = ATT_ERR_INVALID_OFFSET;
    }
    else
    {
      // Copy pValue into the variable we point to from the attribute table.
      memcpy(pAttr->pValue + offset, pValue, len);

      // Only notify application if entire expected value is written
      if ( offset + len == LIPOBATTSERVICE_LIPOBATTHICHAR_LEN)
        paramID = LIPOBATTSERVICE_LIPOBATTHICHAR;
    }
  }
  else
  {
    // If we get here, that means you've forgotten to add an if clause for a
    // characteristic value attribute in the attribute table that has WRITE permissions.
    status = ATT_ERR_ATTR_NOT_FOUND;
  }

  // Let the application know something changed (if it did) by using the
  // callback it registered earlier (if it did).
  if (paramID != 0xFF)
    if ( pAppCBs && pAppCBs->pfnChangeCb )
      pAppCBs->pfnChangeCb( paramID ); // Call app function from stack task context.

  return status;
}
