#include "sensor.h"
#include "inferencia.h"

extern void Delay(uint32_t delay);

float probs[STAI_NETWORK_OUT_1_SIZE];

void Sensor_Init(Sensor_t *sensor, Camera_t *camara){

	sensor->camara = camara;
	sensor->estado = SENSOR_CAPTURA;
	sensor->error = 0;

	sensor->error = ai_init();
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

		sensor->estado = SENSOR_RESULTADO;
		break;

	case SENSOR_RESULTADO:

		Delay (2000);
		sensor->estado = SENSOR_CAPTURA;

		break;

	case SENSOR_ERROR:
		break;
	}
}
