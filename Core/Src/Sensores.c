/*
 * Sensores.c
 *
 *  Created on: Nov 29, 2020
 *      Author: rpaz
 */
#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include "../BSP/stm32l475e_iot01.h"
#include "../BSP/stm32l475e_iot01_accelero.h"
#include "../BSP/stm32l475e_iot01_gyro.h"
#include "../BSP/stm32l475e_iot01_magneto.h"
#include "../BSP/stm32l475e_iot01_hsensor.h"
#include "../BSP/stm32l475e_iot01_psensor.h"
#include "../BSP/stm32l475e_iot01_tsensor.h"
#include "../BSP/stm32l475e_iot01_qspi.h"

TickType_t sensor_period = 100;
/*
float humidity;
float temperature;
float pressure;
int16_t accDataXYZ[3];
float 	gyroDataXYZ[3];
int16_t magDataXYZ[3];
*/
extern QueueHandle_t xQueueSensors;

struct datos mis_datos;

void InitSensores()
{
	BSP_HSENSOR_Init();
	BSP_PSENSOR_Init();
	BSP_TSENSOR_Init();

	BSP_ACCELERO_Init();
	BSP_GYRO_Init();
	BSP_MAGNETO_Init();
}

void ReadSensores()
{
	mis_datos.humidity = BSP_HSENSOR_ReadHumidity();
	mis_datos.pressure = BSP_PSENSOR_ReadPressure();
	mis_datos.temperature = BSP_TSENSOR_ReadTemp();

	BSP_ACCELERO_AccGetXYZ(mis_datos.accDataXYZ);
	BSP_GYRO_GetXYZ(mis_datos.gyroDataXYZ);
	BSP_MAGNETO_GetXYZ(mis_datos.magDataXYZ);
}


void TareaSensores(void *pArg)
{
	InitSensores();
	while(1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		ReadSensores();
		xQueueSend(xQueueSensors,&mis_datos, portMAX_DELAY);
	}
}
