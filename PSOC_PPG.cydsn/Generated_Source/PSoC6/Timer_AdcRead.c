/*******************************************************************************
* File Name: Timer_AdcRead.c
* Version 1.0
*
* Description:
*  This file provides the source code to the API for the Timer_AdcRead
*  component
*
********************************************************************************
* Copyright 2016-2017, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "Timer_AdcRead.h"

/** Indicates whether or not the Timer_AdcRead has been initialized. 
*  The variable is initialized to 0 and set to 1 the first time 
*  Timer_AdcRead_Start() is called. This allows the Component to 
*  restart without reinitialization after the first call to 
*  the Timer_AdcRead_Start() routine.
*/
uint8_t Timer_AdcRead_initVar = 0U;

/** The instance-specific configuration structure. This should be used in the 
*  associated Timer_AdcRead_Init() function.
*/ 
cy_stc_tcpwm_counter_config_t const Timer_AdcRead_config =
{
        .period = 8000UL,
        .clockPrescaler = 0UL,
        .runMode = 0UL,
        .countDirection = 0UL,
        .compareOrCapture = 2UL,
        .compare0 = 16384UL,
        .compare1 = 16384UL,
        .enableCompareSwap = false,
        .interruptSources = 1UL,
        .captureInputMode = 3UL,
        .captureInput = CY_TCPWM_INPUT_CREATOR,
        .reloadInputMode = 3UL,
        .reloadInput = CY_TCPWM_INPUT_CREATOR,
        .startInputMode = 3UL,
        .startInput = CY_TCPWM_INPUT_CREATOR,
        .stopInputMode = 3UL,
        .stopInput = CY_TCPWM_INPUT_CREATOR,
        .countInputMode = 3UL,
        .countInput = CY_TCPWM_INPUT_CREATOR,
};


/*******************************************************************************
* Function Name: Timer_AdcRead_Start
****************************************************************************//**
*
*  Calls the Timer_AdcRead_Init() when called the first time and enables 
*  the Timer_AdcRead. For subsequent calls the configuration is left 
*  unchanged and the component is just enabled.
*
* \globalvars
*  \ref Timer_AdcRead_initVar
*
*******************************************************************************/
void Timer_AdcRead_Start(void)
{
    if (0U == Timer_AdcRead_initVar)
    {
        (void)Cy_TCPWM_Counter_Init(Timer_AdcRead_HW, Timer_AdcRead_CNT_NUM, &Timer_AdcRead_config); 

        Timer_AdcRead_initVar = 1U;
    }

    Cy_TCPWM_Enable_Multiple(Timer_AdcRead_HW, Timer_AdcRead_CNT_MASK);
    
    #if (Timer_AdcRead_INPUT_DISABLED == 7UL)
        Cy_TCPWM_TriggerStart(Timer_AdcRead_HW, Timer_AdcRead_CNT_MASK);
    #endif /* (Timer_AdcRead_INPUT_DISABLED == 7UL) */    
}


/* [] END OF FILE */
