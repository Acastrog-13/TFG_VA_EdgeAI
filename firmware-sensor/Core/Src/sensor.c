#include "sensor.h"
#include "io_modelo.h"
#include "inferencia.h"

extern void Delay(uint32_t delay);

float probs[STAI_NETWORK_OUT_1_SIZE];

void Sensor_Init(Sensor_t *sensor, Camera_t *camara){

	sensor->camara = camara;
	sensor->estado = SENSOR_CAPTURA;
	sensor->error = 0;

	build_lut();

	sensor->error = ai_init();

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET); // LED Azul
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET); // LED Verde
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_RESET); // LED Amarillo
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_RESET); // LED Rojo
}


void Sensor_FSM (Sensor_t *sensor){

	CameraState_t prev = sensor->camara->estado;
	Camera_FSM(sensor->camara);
	CameraState_t sig = sensor->camara->estado;

	 if (sensor->camara->error != Camera_OK || sig == ESTADO_ERROR || sensor->error != 0)
	        sensor->estado = SENSOR_ERROR;

	switch (sensor->estado) {

	case SENSOR_CAPTURA:
		if (prev == ESTADO_CAPTURA && sig == ESTADO_REPOSO)
			sensor->estado = SENSOR_INFERENCIA;
		break;

	case SENSOR_INFERENCIA:

		// Preprocesado
		if (preprocesado(sensor->camara->pBuf, sensor->camara->BufSize, ai_get_input_buffer()) != 0) {

			sensor->estado = SENSOR_ERROR;
			break;
		}

		// Inferencia
		if (inferencia(probs) != 0) {

			sensor->estado = SENSOR_ERROR;
			break;
		}

		// Postprocesado
		sensor->saturacion = postprocesado(probs, 3);

		sensor->estado = SENSOR_RESULTADO;
		break;

	case SENSOR_RESULTADO:

		Delay (2000);
		if (sensor->saturacion == 0)
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);

		else if (sensor->saturacion == 1)
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);

		else if (sensor->saturacion == 2)
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET);

		else if (sensor->saturacion == 3)
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_SET);

		if (HAL_GetTick() - temp >= 1000){

			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_RESET);

			sensor->estado = SENSOR_REPOSO;

		}

		break;

	case SENSOR_ERROR:

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_RESET);

		Delay (1000);

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_SET);

		Delay(1000);

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_RESET);

		Delay (1000);

		sensor->error = 1;

		break;
	}
}
