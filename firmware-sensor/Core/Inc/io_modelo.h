#ifndef INC_IO_MODELO_H_
#define INC_IO_MODELO_H_

#include <stdint.h>


void build_lut();
int8_t preprocesado (const uint8_t *raw_frame, uint32_t raw_frame_size, int8_t *model_input_buf);
int8_t postprocesado(const float *logits, uint32_t num_logits);

#endif /* INC_IO_MODELO_H_ */
