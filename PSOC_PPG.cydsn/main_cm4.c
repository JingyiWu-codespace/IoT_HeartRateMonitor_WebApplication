#include "string.h"
#include <stdio.h>
#include <stdlib.h>

/* Include PSOC generated files */
#include "project.h"

/* Include custom libraries for PPG EduKit */
#include "MAX30105.h"
#include "milliseconds.h"
#include "spo2_algorithm.h"
#include "heartRate.h"
#include "cy_ble_config.h"
#include "cyapicallbacks.h"

/* Constants for the BLE service and characteristic UUIDs */
#define HEART_RATE_SERVICE_UUID         0x180D
#define HEART_RATE_MEASUREMENT_UUID     0x2A37
#define SPO2_MEASUREMENT_UUID           0x2A5F 


/* Activate MAX30105 sensor */
#define USE_MAX_PPG_SENSOR        TRUE
#define MAX_BRIGHTNESS 255
/* Enable UART interface */
#define SERIAL_DEBUG              TRUE

/* Size of averaging buffer for HR and SpO2 measurements */
#define BPM_AVERAGE_RATE          (6U)
#define SPO2_AVERAGE_RATE         (3U)

uint32_t irBuffer[100]; // infrared LED sensor data
uint32_t redBuffer[100];  // red LED sensor data

int32_t bufferLength; // data length
int32_t spo2; // SPO2 value
int8_t validSPO2; // indicator to show if the SPO2 calculation is valid
int32_t heartRate; // heart rate value
int8_t validHeartRate; // indicator to show if the heart rate calculation is valid
int32_t spaceSign;
/* MAX30105 Options */
#define CFG_LED_BRIGHTNESS   (0x1f)
#define CFG_SAMPLE_AVERAGE   (4)
#define CFG_LED_MODE         (2)
#define CFG_SAMPLE_RATE      (100)
#define CFG_PULSE_WIDTH      (411)
#define CFG_ADC_RANGE        (4096)
#define MAX_BUFFER_SIZE (100) 

#define FINGER_THRESHOLD     20000 // Adjust this threshold based on your setup

/* Global variables for BLE connection handle and characteristic handles */
cy_stc_ble_conn_handle_t appConnHandle;
cy_ble_gatt_db_attr_handle_t heartRateCharHandle;
cy_ble_gatt_db_attr_handle_t spo2CharHandle;

/* Function prototypes */
void BLE_callback(uint32_t event, void *eventParam);
void SendHeartRateNotification(uint8_t heartRate);
void SendSpO2Notification(uint8_t spo2);
void UART_PrintData(uint8_t heartRate, uint8_t spo2);
void UpdateHeartRate(uint8_t heartRate);
void UpdateSpo2(uint8_t heartRate);
bool isFingerDetected(uint32_t irValue);




/* BLE Event Callback Function */
void BLE_callback(uint32_t event, void *eventParam)
{
    switch (event)
    {
        case CY_BLE_EVT_STACK_ON:
            printf("Start advertising\n");
            Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);
            break;
        
        case CY_BLE_EVT_GAP_DEVICE_DISCONNECTED:
            printf("Device disconnected!\n");
            Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);
            printf(">>Start advertising!\n");
            break;
            
        case CY_BLE_EVT_GATT_CONNECT_IND:
            printf("Device connected!\n");
            appConnHandle = *(cy_stc_ble_conn_handle_t *)eventParam;
           
            break;
            
        case CY_BLE_EVT_GATT_DISCONNECT_IND:
            memset(&appConnHandle, 0, sizeof(appConnHandle));
            printf("Device disconnected\n");
            Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);
            break;
            
        case CY_BLE_EVT_GATTS_WRITE_REQ:
            Cy_BLE_GATTS_WriteRsp(appConnHandle);
            break;
            
        default:
            break;
    }
}



/* Function to print data to UART */
void UART_PrintData(uint8_t heartRate, uint8_t spo2)
{
    char buffer[50];
    sprintf(buffer, "Heart Rate: %d bpm, SpO2: %d%%\r\n", heartRate, spo2);
    Cy_SCB_UART_PutString(Uart_Printf_HW, buffer);
}

void UART_PrintChar(const char *str)
{
    Cy_SCB_UART_PutString(Uart_Printf_HW, str);
}

void UpdateHeartRate(uint8_t heartRate)
{
    cy_en_ble_api_result_t apiResult;
    
    uint8_t heartRateMeasurement[2];
    heartRateMeasurement[0] = 0x00;
    heartRateMeasurement[1] = heartRate;
    
    apiResult = Cy_BLE_HRSS_SetCharacteristicValue(CY_BLE_HRS_HRM,sizeof(heartRate),&heartRate);
    
    if (apiResult == CY_BLE_SUCCESS)
    {
        printf("Heart Rate update: %d\n", heartRate);
        
        if (Cy_BLE_GetConnectionState(appConnHandle) == CY_BLE_CONN_STATE_CONNECTED)
        {
            apiResult = Cy_BLE_HRSS_SendNotification(appConnHandle, CY_BLE_HRS_HRM, sizeof(heartRateMeasurement), heartRateMeasurement);
            if (apiResult == CY_BLE_SUCCESS)
            {
                printf("Heart Rate notidication sent: %d\n", heartRate);
            }
            else 
            {
                printf("Failed to SEND HR notidication, error code: %d\n",apiResult);
                
            }
        }
        
    }
    else 
    {
        printf(" Failed to update HR, ERROR code:%d\n",apiResult);
    }
    
}

void UpdateSpo2(uint8_t spo2)
{
    cy_en_ble_api_result_t apiResult;
    
    uint8_t heartRateMeasurement[4];
    heartRateMeasurement[0] = 0x08;
    heartRateMeasurement[1] = 0x00;
    heartRateMeasurement[2] = spo2;
    heartRateMeasurement[3] = 0x00;
    
    apiResult = Cy_BLE_HRSS_SetCharacteristicValue(CY_BLE_HRS_HRM,sizeof(heartRateMeasurement),heartRateMeasurement);
    
    if (apiResult == CY_BLE_SUCCESS)
    {
        printf("SPO2 update: %d\n", spo2);
        
        // 检查设备是否连接
        if (Cy_BLE_GetConnectionState(appConnHandle) == CY_BLE_CONN_STATE_CONNECTED)
        {
            // 发送通知
            apiResult = Cy_BLE_HRSS_SendNotification(appConnHandle, CY_BLE_HRS_HRM, sizeof(heartRateMeasurement), heartRateMeasurement);
            if (apiResult == CY_BLE_SUCCESS)
            {
                printf("SPO2 notification sent: %d\n", spo2);
            }
            else 
            {
                printf("Failed to SEND notification, error code: %d\n", apiResult);
            }
        }
    }
    else 
    {
        printf(" Failed to update, ERROR code:%d\n",apiResult);
    }
    
}


void UpdateHeartRateAndSpo2(uint8_t heartRate,uint8_t spo2)
{
    cy_en_ble_api_result_t apiResult;
    
    uint8_t heartRateMeasurement[4];
    heartRateMeasurement[0] = 0x08;
    heartRateMeasurement[1] = heartRate;
    heartRateMeasurement[2] = spo2;
    heartRateMeasurement[3] = 0x00;
    
    apiResult = Cy_BLE_HRSS_SetCharacteristicValue(CY_BLE_HRS_HRM,sizeof(heartRateMeasurement),heartRateMeasurement);
    
    if (apiResult == CY_BLE_SUCCESS)
    {
        printf("Heart Rate update: %d\n", heartRate);
        
        if (Cy_BLE_GetConnectionState(appConnHandle) == CY_BLE_CONN_STATE_CONNECTED)
        {
            apiResult = Cy_BLE_HRSS_SendNotification(appConnHandle, CY_BLE_HRS_HRM, sizeof(heartRateMeasurement), heartRateMeasurement);
            if (apiResult == CY_BLE_SUCCESS)
            {
                printf("Heart Rate and SPO2 notidication sent: HR = %d, SPO2 = %d\n", heartRate,spo2);
            }
            else 
            {
                printf("Failed to SEND notidication, error code: %d\n",apiResult);
                
            }
        }
        
    }
    else 
    {
        printf(" Failed to update, ERROR code:%d\n",apiResult);
    }
    
}


/* Function to check if a finger is detected */
bool isFingerDetected(uint32_t irValue)
{
    return irValue > FINGER_THRESHOLD;
}


/* Main Function */
int main(void)
{   const uint8_t RATE_SIZE = 6; //Increase this for more averaging.
    
    uint8_t rates1[RATE_SIZE]; //Array of heart rates
    uint8_t rateSpot1 = 0;
    long lastBeat1 = 0; //Time at which the last beat occurred
    float beatsPerMinute1;
    int beatAvg1;
    
    
    uint8_t rateSpot2 = 0;
    long lastBeat2 = 0; //Time at which the last beat occurred
    float beatsPerMinute2;
    int beatAvg2;    
    
    /* Assign ISR routines */
    MILLIS_AssignISR();

 
    uint8_t rateSpot = 0;
    long lastBeat = 0;
    float beatsPerMinute;
    int beatAvg;

    int32_t spo2 = 0;
    int8_t validSPO2 = 0;
    int32_t heartRate = 0;
    int8_t validHeartRate = 0;
    
    uint32_t freq = 100; // Example frequency value, set this according to your system
    int32_t peak_distance = 30; 

    uint32_t irBuffer[MAX_BUFFER_SIZE];
    uint32_t redBuffer[MAX_BUFFER_SIZE];
    int32_t bufferSize = MAX_BUFFER_SIZE;

    /* Enable global interrupts */
    __enable_irq(); 

    /* Start timer used for delay function */
    MILLIS_InitAndStartTimer();
    
#if (SERIAL_DEBUG == TRUE)    
    cy_en_scb_uart_status_t uart_status = Cy_SCB_UART_Init(Uart_Printf_HW, &Uart_Printf_config, &Uart_Printf_context);
    if(uart_status != CY_SCB_UART_SUCCESS)
    {
        HandleError();
    }
    Cy_SCB_UART_Enable(Uart_Printf_HW);
#endif    

    /* Start ADC and ADC Conversion */
    //ADC_Start();
    
    Cy_BLE_Start(BLE_callback);
    
    
    while(Cy_BLE_GetState() != CY_BLE_STATE_ON)
    {
        Cy_BLE_ProcessEvents();
    }
   printf(" BLE OPEN !!");

    #if (USE_MAX_PPG_SENSOR == TRUE)
        /* Initialize MAX30105 sensor */
        MAX30105_Setup(CFG_LED_BRIGHTNESS, CFG_SAMPLE_AVERAGE, CFG_LED_MODE, CFG_SAMPLE_RATE, CFG_PULSE_WIDTH, CFG_ADC_RANGE);
    #endif

    while(1)
    {
        Cy_BLE_ProcessEvents();
        CyDelay(100);

        bufferLength = 100; // buffer length of 100 stores 4 seconds of samples running at 25sps

        // Read the first 100 samples, and determine the signal range
        for ( uint8_t i = 0; i < bufferLength; i++)
        {
            while (!MAX30105_Available()) // do we have new data?
                MAX30105_Check(); // Check the sensor for new data

            redBuffer[i] = MAX30105_GetRed();
            irBuffer[i] = MAX30105_GetIR();
            MAX30105_NextSample(); // We're finished with this sample so move to next sample
        }

        // Calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
        maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate, freq, peak_distance);

        // Continuously taking samples from MAX30102. Heart rate and SpO2 are calculated every 1 second
        while (1)
        
        {
            // Dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
            for (uint8_t i = 25; i < 100; i++)
            {
                redBuffer[i - 25] = redBuffer[i];
                irBuffer[i - 25] = irBuffer[i];
            }

            // Take 25 sets of samples before calculating the heart rate.
            for (uint8_t i = 75; i < 100; i++)
            {
                while (!MAX30105_Available()) // do we have new data?
                    MAX30105_Check(); // Check the sensor for new data

                redBuffer[i] = MAX30105_GetRed();
                irBuffer[i] = MAX30105_GetIR();
                MAX30105_NextSample(); // We're finished with this sample so move to next sample
            }

            // Check if a finger is detected
            if (isFingerDetected(irBuffer[bufferLength - 1]))
            {
                // After gathering 25 new samples recalculate HR and SpO2
                maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate, freq, peak_distance);

                // Send BLE notifications
                if (validSPO2 && validHeartRate)
                {
                    //UpdateHeartRate(heartRate);
                    UpdateHeartRateAndSpo2(heartRate,spo2);
                    
                }
                
                if (validSPO2 && validHeartRate)
                {
                    //UpdateHeartRate(heartRate);
                    //int32_t spaceSign = 1;
                    //UpdateHeartRateAndSpo2(spaceSign,spo2);
                    UpdateSpo2(spo2);
                    UART_PrintData((uint8_t)heartRate, (uint8_t)spo2);
                }
            }
            else
            {
                // No finger detected, reset values
                heartRate = 0;
                validHeartRate = 0;
                spo2 = 0;
                validSPO2 = 0;
                
                
                //UpdateHeartRate(heartRate);
                UpdateHeartRateAndSpo2(heartRate,spo2);
                UART_PrintData(heartRate, spo2);
            }
        }

    }
}


