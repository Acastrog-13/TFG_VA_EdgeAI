#ifndef STAI_INFERENCE_INTEGRATION_H
#define STAI_INFERENCE_INTEGRATION_H

#include "network.h"


int8_t ai_init();
int8_t inferencia(float probs_out[STAI_NETWORK_OUT_1_SIZE]);

int8_t *ai_get_input_buffer();

#endif
