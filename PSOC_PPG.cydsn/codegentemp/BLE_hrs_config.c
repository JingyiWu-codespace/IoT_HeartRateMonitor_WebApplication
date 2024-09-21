/***************************************************************************//**
* \file CY_BLE_hrs_config.c
* \version 2.20
* 
* \brief
*  This file contains the source code of initialization of the config structure
*  for the Heart Rate Service of the BLE Component.
*
********************************************************************************
* \copyright
* Copyright 2017-2019, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "ble/cy_ble_hrs.h"

#if(CY_BLE_MODE_PROFILE && defined(CY_BLE_HRS))
#ifdef CY_BLE_HRS_SERVER
static const cy_stc_ble_hrss_t cy_ble_hrss =
{
    0x0010u,    /* Handle of the HRS service */
    {
        0x0012u, /* Handle of the Heart Rate Measurement characteristic */ 
        0x0015u, /* Handle of the Body Sensor Location characteristic */ 
        0x0017u, /* Handle of the Heart Rate Control Point characteristic */ 
    },
    0x0013u,    /* Handle of the Client Characteristic Configuration descriptor */
};
#endif /* CY_BLE_HRS_SERVER */

/**
* \addtogroup group_globals
* @{
*/

/** The configuration structure for the Heart Rate Service. */
cy_stc_ble_hrs_config_t cy_ble_hrsConfig =
{
    /* Service GATT DB handles structure */
    #ifdef CY_BLE_HRS_SERVER
    .hrss = &cy_ble_hrss,
    #else
    .hrss = NULL,
    #endif /* CY_BLE_HRS_SERVER */

    /* An application layer event callback function to receive service events from the BLE Component. */
    .callbackFunc = NULL,
};

/** @} group_globals */
#endif /* (CY_BLE_MODE_PROFILE && defined(CY_BLE_HRS)) */

/* [] END OF FILE */
