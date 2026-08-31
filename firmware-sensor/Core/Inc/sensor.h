#ifndef INC_SENSOR_H_
#define INC_SENSOR_H_

#include "camara.h"

typedef enum {
	SENSOR_CAPTURA,
	SENSOR_INFERENCIA,
	SENSOR_RESULTADO,
	SENSOR_ERROR
}SensorState_t;

typedef struct{
	Camera_t *camara;
	SensorState_t estado;
	int8_t saturacion;
	int8_t error;
}Sensor_t;

void Sensor_Init(Sensor_t *sensor, Camera_t *camara);

void Sensor_FSM (Sensor_t *sensor);

#endif /* INC_SENSOR_H_ */
