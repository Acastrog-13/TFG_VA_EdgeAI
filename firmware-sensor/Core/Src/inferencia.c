#include <stdint.h>
#include <math.h>
#include <string.h>
#include "inferencia.h"

#define AI_NUM_CLASSES   (STAI_NETWORK_OUT_1_SIZE + 1)

// Contexto de la red
STAI_ALIGNED(STAI_NETWORK_CONTEXT_ALIGNMENT)
static uint8_t network_context[STAI_NETWORK_CONTEXT_SIZE];


// Activaciones
STAI_ALIGNED(STAI_NETWORK_ACTIVATION_1_ALIGNMENT)
static uint8_t activations[STAI_NETWORK_ACTIVATION_1_SIZE_BYTES];

static stai_ptr in_ptrs[STAI_NETWORK_IN_NUM];
static stai_ptr out_ptrs[STAI_NETWORK_OUT_NUM];

int8_t ai_init(){
    stai_return_code code;

    code = stai_runtime_init();
    if (code != STAI_SUCCESS) return -1;

    code = stai_network_init((stai_network*)network_context);
    if (code != STAI_SUCCESS) return -1;

    stai_ptr acts[STAI_NETWORK_ACTIVATIONS_NUM] = { activations };
    code = stai_network_set_activations((stai_network*)network_context, acts, STAI_NETWORK_ACTIVATIONS_NUM);
    if (code != STAI_SUCCESS) return -1;

    stai_size n_in = 0, n_out = 0;
    code = stai_network_get_inputs((stai_network*)network_context, in_ptrs, &n_in);
    if (code != STAI_SUCCESS || n_in != STAI_NETWORK_IN_NUM) return -1;

    code = stai_network_get_outputs((stai_network*)network_context, out_ptrs, &n_out);
    if (code != STAI_SUCCESS || n_out != STAI_NETWORK_OUT_NUM) return -1;

    return 0;
}

int8_t inferencia(float probs_out[STAI_NETWORK_OUT_1_SIZE]){

    stai_return_code code;

    code = stai_network_run((stai_network*)network_context, STAI_MODE_SYNC);
    if (code != STAI_SUCCESS) {
        stai_network_get_error((stai_network*)network_context);
        return -1;
    }

    const int8_t *model_out = (const int8_t *)out_ptrs[0];
    for (uint32_t k = 0; k < STAI_NETWORK_OUT_1_SIZE; k++) {
        probs_out[k] = ((float)model_out[k] - STAI_NETWORK_OUT_1_ZERO_POINT)
                       * STAI_NETWORK_OUT_1_SCALE;
    }

    return 0;
}

int8_t *ai_get_input_buffer(){
    return (int8_t *)in_ptrs[0];
}
