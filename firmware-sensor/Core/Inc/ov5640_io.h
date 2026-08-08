#ifndef INC_OV5640_IO_H_
#define INC_OV5640_IO_H_


#define OV5640_I2C_dirESS   0x3C

extern I2C_HandleTypeDef hi2c1;

int32_t OV5640_IO_Init() {
  return 0;
}

int32_t OV5640_IO_DeInit() {
  return (HAL_I2C_DeInit(&hi2c1) == HAL_OK) ? 0 : -1;
}

int32_t OV5640_IO_Write(uint16_t dir, uint16_t reg, uint8_t *pData, uint16_t len) {
  return (HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(dir << 1), reg,
           I2C_MEMADD_SIZE_16BIT, pData, len, 1000) == HAL_OK) ? 0 : -1;
}

int32_t OV5640_IO_Read(uint16_t dir, uint16_t reg, uint8_t *pData, uint16_t len) {
  return (HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(dir << 1), reg,
           I2C_MEMADD_SIZE_16BIT, pData, len, 1000) == HAL_OK) ? 0 : -1;
}

int32_t OV5640_IO_GetTick() {
	return (int32_t)HAL_GetTick();
}


#endif
