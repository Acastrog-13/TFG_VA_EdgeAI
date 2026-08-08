#ifndef INC_CAMARA_H_
#define INC_CAMARA_H_

#include "ov5640.h"
#include "stm32h7xx_hal.h"

#define I2C_ADDRESS 0x3C

//Enumeración de estados de la cámara
typedef enum {
	ESTADO_REPOSO,
	ESTADO_CONFIG,
	ESTADO_CAPTURA,
	ESTADO_ERROR
}CameraState_t;

//Enumeración de errores de la cámara
typedef enum{
	Camera_OK,
	ERR_xclk,
	ERR_ov5640,
	ERR_capture,
	ERR_start,
	ERR_DCMI
}CameraErr_t;

//Estructura de control del modulo de la cámara
typedef struct {
	CameraState_t estado;		//Estado actual de la cámara
	OV5640_Object_t ov5640;		//Sensor ov5640
	DCMI_HandleTypeDef *hdcmi;
	uint32_t capture_period;	//Periodo de captura
	uint8_t *pBuf; 				//Puntero al buffer de imagen
	uint32_t BufSize; 			//Tamaño reservado en RAM
	CameraErr_t error;
	volatile uint8_t flag_capture; //flag del callback de fin de captura
	uint32_t t_capture;
} Camera_t;

//Gestión de los estados de la cámara
void Camera_FSM(Camera_t *cam);

//Inicialización de la cámara
void Camera_Init(Camera_t *cam, DCMI_HandleTypeDef *hdcmi, uint32_t periodo, uint8_t *pBuf, uint32_t BufferSize);

//Secuencia de arranque modulo ov5640
void Camera_Config(Camera_t *cam);

//Inicio y fin de captura
void Camera_StartCapture(Camera_t *cam);
void Camera_StopCapture(Camera_t *cam);

#endif /* INC_CAMARA_H_ */
