#include "io_modelo.h"
#include <math.h>
#include "network.h"

#define CAM_HEIGHT   272
#define CAM_WIDTH    480
#define CROP_SIDE    272
#define MODEL_SIZE   224

#define CROP_X_OFFSET  ((CAM_WIDTH - CROP_SIDE) / 2)


// Tabla de cuantización
static int8_t lut_int8[256];

static int8_t cuantizar_canal(uint8_t valor){

	float real = ((float)valor / 255.0f) * 2.0f - 1.0f;
	int32_t valor_cuant = (int32_t)lroundf(real / STAI_NETWORK_IN_1_SCALE) + STAI_NETWORK_IN_1_ZERO_POINT;

	if (valor_cuant > 127)  valor_cuant = 127;
	if (valor_cuant < -128) valor_cuant = -128;

	return (int8_t)valor_cuant;
}

static void rgb565_to_rgb888 (uint16_t px, uint8_t *r, uint8_t *g, uint8_t *b){

	uint8_t r5 = (px >> 11) & 0x1F;
	uint8_t g6 = (px >> 5)  & 0x3F;
	uint8_t b5 =  px        & 0x1F;

	*r = (uint8_t)((r5 << 3) | (r5 >> 2));
	*g = (uint8_t)((g6 << 2) | (g6 >> 4));
	*b = (uint8_t)((b5 << 3) | (b5 >> 2));

}

static uint16_t recorte(const uint16_t *src16, uint32_t x, uint32_t y){

    uint32_t src_col = x + CROP_X_OFFSET;
    uint32_t src_row = y;

    if (src_col >= CAM_WIDTH)  src_col = CAM_WIDTH  - 1;
    if (src_row >= CAM_HEIGHT) src_row = CAM_HEIGHT - 1;

    return src16[src_row * CAM_WIDTH + src_col];
}


void build_lut(){

	for (int32_t pixel = 0; pixel < 256; pixel++)
		lut_int8[pixel] = cuantizar_canal((uint8_t)pixel);

}


int8_t preprocesado(const uint8_t *imagen, uint32_t imagen_size, int8_t *model_input_buf){

	if (imagen_size < (uint32_t)(CAM_WIDTH * CAM_HEIGHT * 2))
		return -1;

	const float scale = (float)CROP_SIDE / (float)MODEL_SIZE;
	const uint16_t *src16 = (const uint16_t *)imagen;
	uint32_t dst_idx = 0;

	for (uint32_t oy = 0; oy < MODEL_SIZE; oy++) {

		uint32_t sy = (uint32_t)(((float)oy + 0.5f) * scale);

		if (sy 	>= CROP_SIDE) sy = CROP_SIDE - 1;

		for (uint32_t ox = 0; ox < MODEL_SIZE; ox++) {

			uint32_t sx = (uint32_t)(((float)ox + 0.5f) * scale);

			if (sx >= CROP_SIDE) sx = CROP_SIDE - 1;

			uint16_t px = recorte(src16, sx, sy);
			uint8_t r, g, b;

			rgb565_to_rgb888(px, &r, &g, &b);

			model_input_buf[dst_idx++] = lut_int8[r];
			model_input_buf[dst_idx++] = lut_int8[g];
			model_input_buf[dst_idx++] = lut_int8[b];
		}
	}
	return 0;
}


int8_t postprocesado(const float *logits, uint32_t num_logits) {

    int8_t rank = 0;

    for (uint32_t k = 0; k < num_logits; k++)
        if (logits[k] > 0.0f)
        	rank++;

    return rank;
}
