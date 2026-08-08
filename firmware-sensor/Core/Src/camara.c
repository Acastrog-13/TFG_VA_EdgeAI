#include "camara.h"
#include "ov5640_io.h"
#include "main.h"

extern TIM_HandleTypeDef htim2;

void Delay (uint32_t delay) {
    uint32_t tick_inicial = HAL_GetTick();

    while ((HAL_GetTick() - tick_inicial) < delay) {

    }
}

void Camera_FSM (Camera_t *cam){

	if (cam->error != Camera_OK) cam->estado = ESTADO_ERROR;

	switch (cam->estado){

	case ESTADO_REPOSO:

		if (HAL_GetTick() - cam->t_capture > cam->capture_period){
			Camera_StartCapture(cam);
			cam->t_capture = HAL_GetTick();
			cam->estado = ESTADO_CAPTURA;
		}

		break;

	case ESTADO_CONFIG:

		Camera_Config(cam);
		Camera_StartCapture(cam);
		cam->t_capture = HAL_GetTick();
		cam->estado = ESTADO_CAPTURA;

		break;

	case ESTADO_CAPTURA:

		if ((HAL_GetTick() - cam->t_capture) > 5000 ){
			Camera_StopCapture(cam);
			cam->estado = ESTADO_CONFIG;
			break;
		}

		if (cam->flag_capture) {

			SCB_InvalidateDCache_by_Addr((uint32_t*)cam->pBuf, cam->BufSize);
			cam->flag_capture = 0;
			cam->t_capture = HAL_GetTick();
			cam->estado = ESTADO_REPOSO;
		}

		break;

	case ESTADO_ERROR:
		Camera_StopCapture(cam);
		break;

	}
}


void Camera_Init(Camera_t *cam, DCMI_HandleTypeDef *hdcmi, uint32_t periodo, uint8_t *pBuf, uint32_t BufferSize) {

	cam->estado = ESTADO_CONFIG;
	cam->hdcmi = hdcmi;
	cam->capture_period = periodo;
	cam->pBuf = pBuf;
	cam->BufSize = BufferSize;
	cam->error = Camera_OK;
	cam->flag_capture = 0;
	cam->t_capture = 0;

}

void Camera_Config(Camera_t *cam){

	uint32_t id = 0;

	// Reset
	HAL_GPIO_WritePin(GPIOG, RST_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOG, PWDN_Pin, GPIO_PIN_SET);
	Delay(5);

	// Inicio XCLK
	if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1) != HAL_OK){
		cam->error = ERR_xclk;
		return;
	}
	Delay(5);

	// Encendido
	HAL_GPIO_WritePin(GPIOG, PWDN_Pin, GPIO_PIN_RESET);
	Delay(5);

	// Pulso de Reset
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);
	Delay(2);
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);
	Delay(25);

	// Inicialización I2C
	OV5640_IO_t IO = {
			.Init     = OV5640_IO_Init,
			.DeInit   = OV5640_IO_DeInit,
			.Address  = I2C_ADDRESS,
			.WriteReg = OV5640_IO_Write,
			.ReadReg  = OV5640_IO_Read,
			.GetTick  = OV5640_IO_GetTick
	};

	cam->ov5640.Mode = PARALLEL_MODE;

	if (OV5640_RegisterBusIO(&cam->ov5640, &IO) != OV5640_OK) {
		cam->error = ERR_ov5640;
		return;
	}

	if (OV5640_ReadID(&cam->ov5640, &id) != OV5640_OK || id != OV5640_ID) {
		cam->error = ERR_ov5640;
		return;
	}

	if (OV5640_Init(&cam->ov5640, OV5640_R480x272, OV5640_RGB565) != OV5640_OK) {
		cam->error = ERR_ov5640;
		return;
	}

	//Calibracion sensor ov5640
	if (OV5640_Start(&cam->ov5640) != OV5640_OK) {
		cam->error = ERR_ov5640;
		return;
	}

	Delay(500);

	//Comprobación registros
	uint32_t rx = 0U, pclp = 0U, hrefp = 0U, vsyncp = 0U;

	if (OV5640_GetResolution(&cam->ov5640, &rx) ||
			OV5640_GetPolarities(&cam->ov5640, &pclp, &hrefp, &vsyncp)){
		cam->error = ERR_ov5640;
		return;
	}

	else {
		if (rx != OV5640_R480x272 || pclp != OV5640_POLARITY_PCLK_HIGH ||
				hrefp != OV5640_POLARITY_HREF_HIGH || vsyncp != OV5640_POLARITY_VSYNC_HIGH){
			cam->error = ERR_ov5640;
			return;
		}
	}
}

void Camera_StartCapture(Camera_t *cam) {

	cam->flag_capture = 0;

	if (HAL_DCMI_Start_DMA(cam->hdcmi, DCMI_MODE_CONTINUOUS,
						  (uint32_t)cam->pBuf, cam->BufSize / 4) != HAL_OK) {
		cam->error = ERR_start;
		return;
	}

	__HAL_DCMI_ENABLE_IT(cam->hdcmi, DCMI_IT_FRAME | DCMI_IT_OVF | DCMI_IT_ERR);

}

void Camera_StopCapture(Camera_t *cam) {

	if (OV5640_Stop(&cam->ov5640) != OV5640_OK){
		cam->error = ERR_ov5640;
		return;
	}

	if (HAL_DCMI_Stop(cam->hdcmi) != HAL_OK){
		cam->error = ERR_DCMI;
		return;
	}
}

