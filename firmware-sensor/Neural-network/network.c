/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-08-14T11:18:38+0000
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */

#include "ai_lite_inspect.h"
#include "ai_platform_interface.h"
#include "layers.h"
#include "core_convert.h"
#include "network.h"
#include "network_details.h"
#include "network_data.h"
#include "stai_events.h"

#include "lite_operators.h"

#include "ai_lite_inspect.h"
/*****************************************************************************/
#define STAI_INTERNAL_API_MAJOR               (1)
#define STAI_INTERNAL_API_MINOR               (0)
#define STAI_INTERNAL_API_MICRO               (0)

#define STAI_MAGIC                            (0xB1C00100)

/*****************************************************************************/
#define _STAI_CONCAT_ARG(a, b)     a ## b
#define STAI_CONCAT(a, b)         _STAI_CONCAT_ARG(a, b)

/*!  STAI_CAST SECTION                       *********************************/
#define STAI_CAST(type, expr) \
  ((type)(expr))


/*****************************************************************************/
#define STAI_SIZE(_size) \
  ((stai_size)(_size))

/*****************************************************************************/
#define STAI_INIT_BUFFER(_flags, _size, _address) \
  { \
    .size = (_size), \
    .address = (uintptr_t)(_address), \
    .flags = (_flags), \
  }

#define STAI_INIT_TENSOR(_name, _flags, _fmt, _size_bytes, _shape, _scale, _zeropoint) \
  { \
    .size_bytes = (_size_bytes), \
    .flags = (_flags), \
    .format = (stai_format)(_fmt), \
    .shape = STAI_PACK(_shape), \
    .scale = STAI_PACK(_scale), \
    .zeropoint = STAI_PACK(_zeropoint), \
    .name = (_name) \
  }

#define STAI_INIT_ARRAY(_size, _ptr) \
  { .size = STAI_SIZE(_size), .data = STAI_PACK(_ptr) }


#define STAI_CAST_ARRAY(_type, _size, _ptr) \
  { .size = STAI_SIZE(_size), .data = (_type)STAI_PACK(_ptr) }


#define STAI_DECLARE_ARRAY(_type, _size, ...) \
  { .size = STAI_SIZE(_size), .data = (_type[_size]) { STAI_PACK(__VA_ARGS__) } }


#define STAI_EMPTY_ARRAY() \
  { .size = 0, .data = NULL }


#define STAI_INIT_VERSION(_major, _minor, _micro) \
  { .major = (_major), .minor = (_minor), .micro = (_micro), .reserved = 0x0 }

/*****************************************************************************/
/**  Getters and setters  **/

#define STAI_GET_ARRAY_SIZE(nd_array) \
  (nd_array.size)


#define STAI_GET_ARRAY_ELEM(nd_array, pos) \
  (nd_array.data[(pos)])

#define _STAI_SET_ERROR(net_ctx, cond, value, exit) { \
  if (!(net_ctx)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE; } \
  if (((uintptr_t)net_ctx) & (_STAI_CONTEXT_ALIGNMENT-1)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_ALIGNMENT; } \
  if (((value) >= STAI_ERROR_GENERIC) && (cond)) { \
    if ((net_ctx)->_return_code == STAI_SUCCESS) { \
      (net_ctx)->_return_code = (value); \
    } \
    return (exit); \
  } \
}

/*****************************************************************************/
/* TODO REMOVE THESE TWO MACROS */
#define STAI_EVENT_NODE_START_CB
#define STAI_EVENT_NODE_STOP_CB

#ifdef STAI_EVENT_NODE_START_CB
#ifndef _STAI_NETWORK_EVENT_NODE_START_CB
  #define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _start_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(const stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_START, (const void*)&_start_event); \
  }
#endif
#else
  #define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_NETWORK_EVENT_NODE_START_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_START_CB */

#ifdef STAI_EVENT_NODE_STOP_CB
#ifndef _STAI_NETWORK_EVENT_NODE_STOP_CB
  #define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _stop_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_STOP, (const void*)&_stop_event); \
  }
#endif
#else
  #define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_NETWORK_EVENT_NODE_STOP_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_STOP_CB */


/*****************************************************************************/
#define _STAI_NETWORK_MODEL_SIGNATURE     "0x877f9dfe4819c5ba09bfd64af65e6960"
#define _STAI_NETWORK_DATETIME            "2026-08-14T11:18:38+0000"
#define _STAI_NETWORK_COMPILE_DATETIME    __DATE__ " " __TIME__

#define _STAI_CONTEXT_ALIGNMENT        STAI_NETWORK_CONTEXT_ALIGNMENT

/*****************************************************************************/
#define g_network_activations_1     (NULL)




#if defined(HAVE_NETWORK_INFO)
/*****************************************************************************/
static const stai_network_info g_network_info = {
  .model_signature = _STAI_NETWORK_MODEL_SIGNATURE,
  .c_compile_datetime = _STAI_NETWORK_COMPILE_DATETIME,
  .c_model_name = STAI_NETWORK_MODEL_NAME,
  .c_model_datetime = _STAI_NETWORK_DATETIME,
  .c_model_signature = 0x0,
  .runtime_version = STAI_INIT_VERSION(12, 0, 1),
  .tool_version = STAI_INIT_VERSION(4, 0, 1),
  .api_version = STAI_INIT_VERSION(1, 0, 0),
  .n_macc = STAI_NETWORK_MACC_NUM,
  .n_nodes = STAI_NETWORK_NODES_NUM,
  .flags = STAI_NETWORK_FLAGS,
  .n_inputs = STAI_NETWORK_IN_NUM,
  .n_outputs = STAI_NETWORK_OUT_NUM,
  .n_activations = STAI_NETWORK_ACTIVATIONS_NUM,
  .n_weights = STAI_NETWORK_WEIGHTS_NUM,
  .n_states = STAI_NETWORK_STATES_NUM,
  .inputs = (stai_tensor[STAI_NETWORK_IN_NUM]) {
    STAI_INIT_TENSOR(
      STAI_NETWORK_IN_1_NAME,
      STAI_NETWORK_IN_1_FLAGS,
      STAI_NETWORK_IN_1_FORMAT,
      STAI_NETWORK_IN_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 224, 224, 3),
      STAI_DECLARE_ARRAY(float, 1, 0.007843137718737125f),
      STAI_DECLARE_ARRAY(int16_t, 1, -1)),
    },
    .outputs = (stai_tensor[STAI_NETWORK_OUT_NUM]) {
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_1_NAME,
      STAI_NETWORK_OUT_1_FLAGS,
      STAI_NETWORK_OUT_1_FORMAT,
      STAI_NETWORK_OUT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 2, 1, 3),
      STAI_DECLARE_ARRAY(float, 1, 0.058987729251384735f),
      STAI_DECLARE_ARRAY(int16_t, 1, 14)),
    },
  .activations = (stai_tensor[STAI_NETWORK_ACTIVATIONS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_ACTIVATION_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_ACTIVATION_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 421248),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
  .weights = (stai_tensor[STAI_NETWORK_WEIGHTS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 895720),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },

  .states = NULL
};
#endif

#define _STAI_CONTEXT_ACQUIRE(_net_ctx, _net_handle) \
  _stai_network_context* _net_ctx = (_stai_network_context*)(_net_handle); \
  STAI_ASSERT(_net_ctx != NULL) \
  _STAI_SET_ERROR(_net_ctx, _net_ctx->_magic != STAI_MAGIC, \
                  STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE, _net_ctx->_return_code)


/*****************************************************************************/
static
void _stai_network_check(_stai_network_context* net_ctx)
{
  stai_size idx;

// Check activations status
  for (idx=0; idx<STAI_NETWORK_ACTIVATIONS_NUM; idx++) {
    if (net_ctx->_activations[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_ACTIVATIONS_NUM) ? STAI_FLAG_ACTIVATIONS : STAI_FLAG_NONE;
// Check inputs status
  for (idx=0; idx<STAI_NETWORK_IN_NUM; idx++) {
    if (net_ctx->_inputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_IN_NUM) ? STAI_FLAG_INPUTS : STAI_FLAG_NONE;

  // Check outputs status
  for (idx=0; idx<STAI_NETWORK_OUT_NUM; idx++) {
    if (net_ctx->_outputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_OUT_NUM) ? STAI_FLAG_OUTPUTS : STAI_FLAG_NONE;

// Check weights status
  for (idx=0; idx<STAI_NETWORK_WEIGHTS_NUM; idx++) {
    if (net_ctx->_weights[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_WEIGHTS_NUM) ? STAI_FLAG_WEIGHTS : STAI_FLAG_NONE;
STAI_PRINT("  [_stai_network_check] flags: 0x%08x\n", net_ctx->_flags)
}


/*****************************************************************************/
STAI_API_ENTRY
stai_return_code stai_network_init(
  stai_network* network)
{
  /* Memory where to store internal context is provided by applications as a raw byte buffer */
  _stai_network_context* net_ctx = (_stai_network_context*)(network);
  net_ctx->_return_code = STAI_SUCCESS;
  STAI_PRINT("[Entering Network Init] network(%p) context_size(%d)\n", net_ctx, (int32_t)sizeof(_stai_network_context))

  _STAI_SET_ERROR(net_ctx, STAI_NETWORK_CONTEXT_SIZE != sizeof(_stai_network_context),
                 STAI_ERROR_NETWORK_INVALID_CONTEXT_SIZE, net_ctx->_return_code)

  {
    const _stai_network_context _network_context = {
      ._magic = STAI_MAGIC,
      ._signature = STAI_NETWORK_MODEL_SIGNATURE,
      ._flags = STAI_NETWORK_FLAGS,
      ._return_code = STAI_SUCCESS,
      ._callback = NULL,
      ._callback_cookie = NULL,
      ._activations = {
      (stai_ptr)g_network_activations_1
      },
      ._weights = {
      (stai_ptr)g_network_weights_array
      },
      ._inputs = {
    NULL},
      ._outputs = {
    NULL},
    };

    // Deep copy of internal context to opaque buffer provided by app
    *net_ctx = _network_context;

    _stai_network_check(net_ctx);
  }

  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_deinit(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /*  Reset flags to initial state  */
  net_ctx->_flags = STAI_NETWORK_FLAGS;
  return net_ctx->_return_code;
}

/*****************************************************************************/



/* Int quant #0 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_26_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0235294122248888f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(pool_27_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.01859661005437374f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_28_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.030651606619358063f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_28_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 128,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0012940957676619291f, 0.0020027304999530315f, 0.001098566921427846f, 0.0008953239303082228f, 0.0009270849986933172f, 0.0016918061301112175f, 0.0010808835504576564f, 0.0020942185074090958f, 0.0012550008250400424f, 0.0017378285992890596f, 0.0016588906291872263f, 0.0015442578587681055f, 0.0015862603904679418f, 0.0015361909754574299f, 0.0014563133008778095f, 0.0012125385692343116f, 0.0015292264288291335f, 0.0016133071621879935f, 0.0009206146351061761f, 0.0013411674881353974f, 0.0009386262972839177f, 0.0019435621798038483f, 0.0017784835072234273f, 0.001727394643239677f, 0.002029950264841318f, 0.001984853297472f, 0.0017530032200738788f, 0.0010107588022947311f, 0.0010805571218952537f, 0.0016874232096597552f, 0.0019566381815820932f, 0.0010618080850690603f, 0.001559320604428649f, 0.0009556509321555495f, 0.001394531223922968f, 0.0014546349411830306f, 0.0013330212095752358f, 0.0017763456562533975f, 0.001547227962873876f, 0.0009478760766796768f, 0.0015183205250650644f, 0.000973996240645647f, 0.0013123556273058057f, 0.0009524801280349493f, 0.0009089410305023193f, 0.0015587648376822472f, 0.0014894602354615927f, 0.0009597946773283184f, 0.0009557789890095592f, 0.0014558433322235942f, 0.002009883988648653f, 0.0022245021536946297f, 0.0019771794322878122f, 0.0021399925462901592f, 0.0008891740581020713f, 0.00173109769821167f, 0.0014216202544048429f, 0.0015448437770828605f, 0.0018266425468027592f, 0.0013294513337314129f, 0.0019767466001212597f, 0.0019168900325894356f, 0.001239890931174159f, 0.0014478573575615883f, 0.001127291121520102f, 0.0021606083028018475f, 0.0020330040715634823f, 0.0017586590256541967f, 0.0010118153877556324f, 0.001148607232607901f, 0.0022732524666935205f, 0.0015864060260355473f, 0.0010178153170272708f, 0.0016539102653041482f, 0.0020735471043735743f, 0.0009180853958241642f, 0.0016849406529217958f, 0.0017189766513183713f, 0.0009144213399849832f, 0.000927307759411633f, 0.0014830280561000109f, 0.0015357136726379395f, 0.0017306443769484758f, 0.0015220640925690532f, 0.0018053565872833133f, 0.0019424234051257372f, 0.0016177388606593013f, 0.0014029318699613214f, 0.0015223089139908552f, 0.0022948337718844414f, 0.0012241075746715069f, 0.0014418945647776127f, 0.0009435332613065839f, 0.0015494442777708173f, 0.0009925171034410596f, 0.0012012528022751212f, 0.0018315095221623778f, 0.0012362487614154816f, 0.001974558923393488f, 0.00133992126211524f, 0.0011435030028223991f, 0.0013260326813906431f, 0.0013618298107758164f, 0.0018548965454101562f, 0.0014247801154851913f, 0.0013814877020195127f, 0.0017189007485285401f, 0.0020950695034116507f, 0.0008804843528196216f, 0.0011855493066832423f, 0.002272471087053418f, 0.001152115873992443f, 0.001054532011039555f, 0.0011119770351797342f, 0.0012882453156635165f, 0.001446224283427f, 0.001664076466113329f, 0.0009861728176474571f, 0.0008928373572416604f, 0.002263969974592328f, 0.0018251750152558088f, 0.0011977559188380837f, 0.0009989341488108039f, 0.0013964528916403651f, 0.001119697350077331f, 0.0017563289729878306f, 0.001639627618715167f, 0.002242164919152856f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))

/* Int quant #4 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_29_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.05076202005147934f),
    AI_PACK_INTQ_ZP(16)))

/* Int quant #5 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_30_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.058987729251384735f),
    AI_PACK_INTQ_ZP(14)))

/* Int quant #6 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(tfl_pseudo_qconst_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00822571199387312f),
    AI_PACK_INTQ_ZP(-2)))



/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_26_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 25088, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  pool_27_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 512, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  gemm_28_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 128, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  gemm_28_weights_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 65536, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  gemm_28_bias_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 128, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  gemm_28_scratch0_array, AI_ARRAY_FORMAT_S16,
  NULL, NULL, 1152, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  gemm_29_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_30_output_array, AI_ARRAY_FORMAT_S8|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 3, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  tfl_pseudo_qconst_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3, AI_STATIC)



/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_26_output, AI_STATIC,
  82, 0x1,
  AI_SHAPE_INIT(4, 1, 512, 7, 7), AI_STRIDE_INIT(4, 1, 1, 512, 3584),
  1, &conv2d_26_output_array, &conv2d_26_output_array_intq)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  pool_27_output, AI_STATIC,
  130, 0x1,
  AI_SHAPE_INIT(4, 1, 512, 1, 1), AI_STRIDE_INIT(4, 1, 1, 512, 512),
  1, &pool_27_output_array, &pool_27_output_array_intq)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  gemm_28_bias, AI_STATIC,
  122, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 4, 4, 512, 512),
  1, &gemm_28_bias_array, NULL)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  gemm_28_output, AI_STATIC,
  123, 0x1,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 1, 1, 128, 128),
  1, &gemm_28_output_array, &gemm_28_output_array_intq)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  gemm_28_scratch0, AI_STATIC,
  124, 0x0,
  AI_SHAPE_INIT(4, 1, 1152, 1, 1), AI_STRIDE_INIT(4, 2, 2, 2304, 2304),
  1, &gemm_28_scratch0_array, NULL)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  gemm_28_weights, AI_STATIC,
  125, 0x1,
  AI_SHAPE_INIT(4, 512, 128, 1, 1), AI_STRIDE_INIT(4, 1, 512, 65536, 65536),
  1, &gemm_28_weights_array, &gemm_28_weights_array_intq)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_30_output, AI_STATIC,
  121, 0x1,
  AI_SHAPE_INIT(4, 1, 3, 1, 1), AI_STRIDE_INIT(4, 1, 1, 3, 3),
  1, &eltwise_30_output_array, &eltwise_30_output_array_intq)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  gemm_29_output, AI_STATIC,
  127, 0x1,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 1, 1, 1, 1),
  1, &gemm_29_output_array, &gemm_29_output_array_intq)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  tfl_pseudo_qconst, AI_STATIC,
  132, 0x1,
  AI_SHAPE_INIT(4, 1, 3, 1, 1), AI_STRIDE_INIT(4, 1, 1, 3, 3),
  1, &tfl_pseudo_qconst_array, &tfl_pseudo_qconst_array_intq)


AI_TENSOR_CHAIN_OBJ_DECLARE(
  pool_27_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_26_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_27_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  pool_27_layer, 27,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap_integer_INT8,
  &pool_27_chain,
  NULL, &pool_27_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(7, 7), 
  .pool_stride = AI_SHAPE_2D_INIT(7, 7), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  gemm_28_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &pool_27_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_28_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &gemm_28_weights, &gemm_28_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_28_scratch0)
)

AI_LAYER_OBJ_DECLARE(
  gemm_28_layer, 28,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense_integer_SSSA_ch,
  &gemm_28_chain,
  NULL, &gemm_28_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_30_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &gemm_29_output, &tfl_pseudo_qconst),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_30_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_30_layer, 30,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_30_chain,
  NULL, &eltwise_30_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)
/**  Hybrid layers declarations section  *************************************/
void forward_lite_ap_integer_INT8_pool_27(_stai_network_context* net_ctx)
{
  conv2d_26_output_array.data = AI_PTR(net_ctx->_activations[0] + 44036);
  conv2d_26_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 44036);
  pool_27_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_27_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(27, 1, { conv2d_26_output.data->data});
  forward_ap_integer_INT8(&pool_27_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(27, 1, { pool_27_output.data->data});
}
void forward_lite_dense_integer_SSSA_ch_gemm_28(_stai_network_context* net_ctx)
{
  pool_27_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  pool_27_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  gemm_28_weights_array.data = AI_PTR(net_ctx->_weights[0] + 829540);
  gemm_28_weights_array.data_start = AI_PTR(net_ctx->_weights[0] + 829540);
  gemm_28_bias_array.data = AI_PTR(net_ctx->_weights[0] + 895076);
  gemm_28_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 895076);
  gemm_28_scratch0_array.data = AI_PTR(net_ctx->_activations[0] + 512);
  gemm_28_scratch0_array.data_start = AI_PTR(net_ctx->_activations[0] + 512);
  gemm_28_output_array.data = AI_PTR(net_ctx->_activations[0] + 2816);
  gemm_28_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 2816);
  _STAI_NETWORK_EVENT_NODE_START_CB(28, 1, { pool_27_output.data->data});
  forward_dense_integer_SSSA_ch(&gemm_28_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(28, 1, { gemm_28_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_30(_stai_network_context* net_ctx)
{
  gemm_29_output_array.data = AI_PTR(net_ctx->_activations[0] + 256);
  gemm_29_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 256);
  tfl_pseudo_qconst_array.data = AI_PTR(net_ctx->_weights[0] + 0);
  tfl_pseudo_qconst_array.data_start = AI_PTR(net_ctx->_weights[0] + 0);
  eltwise_30_output_array.data = AI_PTR(net_ctx->_outputs[0] + 0);
  eltwise_30_output_array.data_start = AI_PTR(net_ctx->_outputs[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(30, 2, { gemm_29_output.data->data,tfl_pseudo_qconst.data->data});
  forward_eltwise_integer_INT8(&eltwise_30_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(30, 1, { eltwise_30_output.data->data});
}

/*****************************************************************************/


static const ai_u16 conv2d_0_t_in_0_shape_w_const_u16 = 224;
static const ai_u16 conv2d_0_t_out_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_0_t_weight_0_shape_w_const_u16 = 3;
static const ai_i32 conv2d_0_l_pad_W_0_const_s32 = 0;
static const ai_u16 conv2d_0_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_0_t_in_0_fmt_zero_const_s8 = -1;
static const ai_i8 conv2d_0_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_0_t_in_0_fmt_scale_const_f32 = 0.007843137718737125f;
static const ai_float conv2d_0_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_0_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0030361381359398365f, 0.0030744331888854504f, 0.029156280681490898f, 0.006554547697305679f, 1.7555051101680874e-07f, 0.0037365693133324385f, 0.008266227319836617f, 2.1348093071082985e-07f, 0.003167781513184309f, 0.0035228498745709658f, 2.3300441398532712e-07f, 0.0067278193309903145f, 0.008499221876263618f, 4.414243335304491e-07f, 0.003261030185967684f, 2.152276579181489e-07f);
static const ai_layer_format_type conv2d_0_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_0_t_out_0_shape_w_const_u16 = 112;

static const ai_i8 conv2d_1_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_1_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_1_pad_before_t_in_0_shape_h_const_u32 = 112;

static const ai_u16 conv2d_1_t_in_0_shape_w_const_u16 = 114;
static const ai_u16 conv2d_1_t_in_0_shape_h_const_u16 = 114;
static const ai_u16 conv2d_1_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_1_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_1_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_1_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_1_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_1_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_1_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.012296298518776894f, 0.2179647833108902f, 0.016549335792660713f, 0.022059014067053795f, 0.2663106620311737f, 0.012135437689721584f, 0.01700923964381218f, 0.696378767490387f, 0.07989521324634552f, 0.020184848457574844f, 0.31660887598991394f, 0.014110143296420574f, 0.02059589885175228f, 0.27971920371055603f, 0.06686202436685562f, 0.31662657856941223f);
static const ai_u16 conv2d_1_t_out_0_shape_w_const_u16 = 112;
static const ai_u16 conv2d_1_t_out_0_shape_h_const_u16 = 112;

static const ai_u16 conv2d_2_t_in_0_shape_w_const_u16 = 112;
static const ai_u16 conv2d_2_t_in_0_shape_h_const_u16 = 112;
static const ai_u16 conv2d_2_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_2_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_2_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_2_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_2_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_2_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_2_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_2_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_2_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004239371046423912f, 0.005704953800886869f, 0.023296019062399864f, 0.004115171264857054f, 0.0010753205278888345f, 0.003821558551862836f, 0.02971726283431053f, 0.003024306381121278f, 0.016730334609746933f, 0.004354786593466997f, 0.0070137279108166695f, 0.002002225024625659f, 0.005388996098190546f, 0.006319193635135889f, 0.0038624745793640614f, 0.007696972228586674f, 0.008257800713181496f, 0.0030020796693861485f, 0.0062232851050794125f, 0.030196422711014748f, 0.008033999241888523f, 0.006598911248147488f, 0.001074539264664054f, 0.006509782280772924f, 0.004145333077758551f, 0.005511074792593718f, 0.005166671238839626f, 0.009751171804964542f, 0.016583619639277458f, 0.0051756990142166615f, 0.016640041023492813f, 0.01246272400021553f);
static const ai_layer_format_type conv2d_2_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_3_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_3_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_3_pad_before_t_in_0_shape_h_const_u32 = 112;

static const ai_u16 conv2d_3_t_in_0_shape_w_const_u16 = 114;
static const ai_u16 conv2d_3_t_in_0_shape_h_const_u16 = 114;
static const ai_u16 conv2d_3_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_3_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_3_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_3_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_3_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_3_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_3_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_3_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0043547493405640125f, 0.00823222752660513f, 0.0018561874749138951f, 0.024743862450122833f, 0.023805104196071625f, 0.00832684338092804f, 0.0005009484011679888f, 0.005646429490298033f, 0.0032450042199343443f, 0.004538742825388908f, 0.002325408160686493f, 0.00771500077098608f, 0.00520762475207448f, 0.005685179959982634f, 0.007365046069025993f, 0.002281538676470518f, 0.002233361592516303f, 0.008387641981244087f, 0.0009746722644194961f, 0.0006390720373019576f, 0.0022399802692234516f, 0.00329041201621294f, 0.007936841808259487f, 0.0026653343811631203f, 0.0054720076732337475f, 0.00686462689191103f, 0.002915194258093834f, 0.002511749044060707f, 0.0015493982937186956f, 0.0036317685153335333f, 0.0028040450997650623f, 0.0013029560213908553f);
static const ai_u16 conv2d_3_t_out_0_shape_w_const_u16 = 56;
static const ai_u16 conv2d_3_t_out_0_shape_h_const_u16 = 56;

static const ai_u16 conv2d_4_t_in_0_shape_w_const_u16 = 56;
static const ai_u16 conv2d_4_t_in_0_shape_h_const_u16 = 56;
static const ai_u16 conv2d_4_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_4_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_4_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_4_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_4_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_4_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_4_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_4_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_4_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.007028673309832811f, 0.0024699601344764233f, 0.0045356410555541515f, 0.0025072600692510605f, 0.004882228095084429f, 0.013599129393696785f, 0.008324824273586273f, 0.006281248759478331f, 0.004008251242339611f, 6.002903774060542e-08f, 0.0032634909730404615f, 0.0031992734875530005f, 0.005114623345434666f, 0.004851602949202061f, 0.002069124486297369f, 0.0038917306810617447f, 0.00839747954159975f, 0.0009303448023274541f, 0.005160689819604158f, 0.006031414959579706f, 0.010829887352883816f, 0.0068771070800721645f, 0.0036464333534240723f, 0.0042993961833417416f, 0.00580750172957778f, 0.0038434043526649475f, 0.008197878487408161f, 0.004079775419086218f, 0.004690221045166254f, 0.006965312175452709f, 0.003876789938658476f, 0.00363004207611084f, 0.005330206826329231f, 0.0073079802095890045f, 0.004229946993291378f, 0.014347531832754612f, 0.008114668540656567f, 0.0032006173860281706f, 7.193528261950632e-08f, 0.012869095429778099f, 0.031630732119083405f, 0.005535824690014124f, 0.004413953050971031f, 0.005308943334966898f, 0.004962096456438303f, 0.0027797247748821974f, 0.004170900210738182f, 0.003754608565941453f, 0.005371225997805595f, 0.031155265867710114f, 0.0036392270121723413f, 0.004008488263934851f, 8.888996205769217e-08f, 0.002661993494257331f, 0.018727222457528114f, 0.004803340416401625f, 0.006526832934468985f, 0.003788798814639449f, 0.00350079289637506f, 0.006125203333795071f, 0.030271107330918312f, 0.0017829869175329804f, 0.00818874966353178f, 3.937008052901092e-09f);
static const ai_layer_format_type conv2d_4_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_5_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_5_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_5_pad_before_t_in_0_shape_h_const_u32 = 56;

static const ai_u16 conv2d_5_t_in_0_shape_w_const_u16 = 58;
static const ai_u16 conv2d_5_t_in_0_shape_h_const_u16 = 58;
static const ai_u16 conv2d_5_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_5_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_5_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_5_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_5_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_5_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_5_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_5_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.011645818129181862f, 0.04691033437848091f, 0.009340833872556686f, 0.03350871428847313f, 0.01801995374262333f, 0.008630926720798016f, 0.028472721576690674f, 0.0065387883223593235f, 0.01801767759025097f, 0.13121937215328217f, 0.08332846313714981f, 0.022105026990175247f, 0.03273746743798256f, 0.013107777573168278f, 0.03153665363788605f, 0.013111653737723827f, 0.007784348446875811f, 0.07410255819559097f, 0.03024331107735634f, 0.02088434062898159f, 0.007137455511838198f, 0.007725466042757034f, 0.014788920991122723f, 0.03499643877148628f, 0.012367168441414833f, 0.02965969406068325f, 0.01068174373358488f, 0.040847111493349075f, 0.01426719781011343f, 0.009255914017558098f, 0.03212350979447365f, 0.014864974655210972f, 0.009066184051334858f, 0.009503301233053207f, 0.03362137824296951f, 0.002596387406811118f, 0.011728094890713692f, 0.028192570433020592f, 0.18554188311100006f, 0.01609901711344719f, 0.00766643974930048f, 0.019312400370836258f, 0.02783612161874771f, 0.0192415714263916f, 0.0174131877720356f, 0.021922126412391663f, 0.020447496324777603f, 0.014249700121581554f, 0.050833117216825485f, 0.00573183735832572f, 0.0496244877576828f, 0.014225701801478863f, 0.5112985968589783f, 0.029724672436714172f, 0.0073418752290308475f, 0.03548659011721611f, 0.013876284472644329f, 0.036192476749420166f, 0.04014136269688606f, 0.004791564308106899f, 0.005692081060260534f, 0.027174556627869606f, 0.005344880744814873f, 0.011804656125605106f);
static const ai_u16 conv2d_5_t_out_0_shape_w_const_u16 = 56;
static const ai_u16 conv2d_5_t_out_0_shape_h_const_u16 = 56;

static const ai_u16 conv2d_6_t_in_0_shape_w_const_u16 = 56;
static const ai_u16 conv2d_6_t_in_0_shape_h_const_u16 = 56;
static const ai_u16 conv2d_6_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_6_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_6_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_6_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_6_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_6_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_6_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_6_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_6_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.011874516494572163f, 0.003226710017770529f, 0.013389057479798794f, 0.006738395430147648f, 0.0026824637316167355f, 0.011508869007229805f, 0.004642987158149481f, 0.005422974471002817f, 0.004751160740852356f, 0.004180906340479851f, 0.0028308334294706583f, 0.004941632505506277f, 0.004436633083969355f, 0.019214117899537086f, 0.003902199910953641f, 0.0026197994593530893f, 0.0021069119684398174f, 0.012101879343390465f, 0.0021118056029081345f, 0.004350719042122364f, 0.0038547860458493233f, 0.0014013664331287146f, 0.005563994403928518f, 0.004931551869958639f, 0.004062659572809935f, 0.009579171426594257f, 0.004180414602160454f, 0.002759587951004505f, 0.0019959069322794676f, 0.005856651347130537f, 0.0059516518376767635f, 0.001901333569549024f, 0.007381313480436802f, 0.007373869884759188f, 0.01770593412220478f, 0.0012560853501781821f, 0.0028082849457859993f, 0.005199294537305832f, 0.002580601954832673f, 0.00868212804198265f, 0.020358607172966003f, 0.06618968397378922f, 0.013739150948822498f, 0.017331890761852264f, 0.003077371045947075f, 0.007518972270190716f, 0.004455003887414932f, 0.0036958346609026194f, 0.005310585722327232f, 0.003059219568967819f, 0.006177166476845741f, 0.004203625954687595f, 0.0074013229459524155f, 0.0036987881176173687f, 0.006871344521641731f, 0.0036365813575685024f, 0.0019470336847007275f, 0.011386403813958168f, 0.012073279358446598f, 0.005174335092306137f, 0.022264691069722176f, 0.00555932754650712f, 0.002365557476878166f, 0.003761283354833722f);
static const ai_layer_format_type conv2d_6_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_7_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_7_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_7_pad_before_t_in_0_shape_h_const_u32 = 56;

static const ai_u16 conv2d_7_t_in_0_shape_w_const_u16 = 58;
static const ai_u16 conv2d_7_t_in_0_shape_h_const_u16 = 58;
static const ai_u16 conv2d_7_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_7_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_7_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_7_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_7_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_7_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_7_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_7_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0022201065439730883f, 0.008615823462605476f, 0.001725956448353827f, 0.0027401756960898638f, 0.004698099568486214f, 0.002691454952582717f, 0.004537545144557953f, 0.0018233275040984154f, 0.003037120681256056f, 0.0048576644621789455f, 0.003954202402383089f, 0.0038805603981018066f, 0.005209994036704302f, 0.0008580716676078737f, 0.00578986806795001f, 0.008626281283795834f, 0.002835404360666871f, 0.0012920070439577103f, 0.011028102599084377f, 0.0027624478098005056f, 0.005579647142440081f, 0.0050793299451470375f, 0.003144501941278577f, 0.004836017731577158f, 0.005593876354396343f, 0.0020429175347089767f, 0.0023901890963315964f, 0.010718678124248981f, 0.006667050067335367f, 0.001722607878036797f, 0.0018766949651762843f, 0.010980943217873573f, 0.0038256861735135317f, 0.0034364224411547184f, 0.0018789173336699605f, 0.007281145546585321f, 0.004610054660588503f, 0.0048529645428061485f, 0.009861141443252563f, 0.004848537500947714f, 0.0014111277414485812f, 0.0012246378464624286f, 0.004597811494022608f, 0.0016129398718476295f, 0.004482271149754524f, 0.0039923302829265594f, 0.00718814879655838f, 0.0033816529903560877f, 0.002445981139317155f, 0.0035824959632009268f, 0.002345228800550103f, 0.00303706550039351f, 0.0016890274127945304f, 0.007851307280361652f, 0.004224501084536314f, 0.005035186652094126f, 0.007144874893128872f, 0.0017563595902174711f, 0.004429738037288189f, 0.002600074280053377f, 0.001262865262106061f, 0.0033411516342312098f, 0.014880750328302383f, 0.006223534233868122f);
static const ai_u16 conv2d_7_t_out_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_7_t_out_0_shape_h_const_u16 = 28;

static const ai_u16 conv2d_8_t_in_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_8_t_in_0_shape_h_const_u16 = 28;
static const ai_u16 conv2d_8_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_8_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_8_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_8_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_8_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_8_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_8_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_8_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_8_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0027519152499735355f, 0.005172572564333677f, 0.0049572051502764225f, 0.0062346793711185455f, 0.0022777444683015347f, 0.003650354454293847f, 0.002876642160117626f, 0.005799705162644386f, 0.0040272860787808895f, 0.0018506167689338326f, 0.005205685272812843f, 0.004151277244091034f, 0.006577839143574238f, 0.003089878475293517f, 0.0017070609610527754f, 0.0045639462769031525f, 0.002741487929597497f, 0.003219946287572384f, 0.00448711309581995f, 0.004382173530757427f, 0.005103020928800106f, 0.0033206737134605646f, 0.0038632466457784176f, 0.003917767200618982f, 0.0016827649669721723f, 0.0054538254626095295f, 0.003981613088399172f, 0.008712377399206161f, 0.003197710495442152f, 0.003061346709728241f, 0.0030731302686035633f, 0.002681043231859803f, 0.007181461434811354f, 0.0028944124933332205f, 0.005064220633357763f, 0.0028214023914188147f, 0.0024369314778596163f, 0.0151537349447608f, 0.002677477430552244f, 0.003510416718199849f, 0.005299135576933622f, 0.0058900476433336735f, 0.0033600199967622757f, 0.00489441491663456f, 0.004797527566552162f, 0.0069012208841741085f, 0.006021446548402309f, 0.0026098445523530245f, 0.002032446675002575f, 0.003741039428859949f, 0.0029544541612267494f, 0.0033129393123090267f, 0.005803393200039864f, 0.0017327230889350176f, 0.003935104701668024f, 0.005630651954561472f, 0.005969023331999779f, 0.004000735469162464f, 0.0032658760901540518f, 0.008424355648458004f, 0.0028078057803213596f, 0.002634207019582391f, 0.006392181850969791f, 0.007864909246563911f, 0.002784483600407839f, 0.0032091205939650536f, 0.007272796239703894f, 0.00367468548938632f, 0.006222843658179045f, 0.003183844266459346f, 0.005483980756253004f, 0.0031952550634741783f, 0.010970494709908962f, 0.004760705400258303f, 0.0033309864811599255f, 0.00838739238679409f, 0.002066192450001836f, 0.0020393074955791235f, 0.002047385787591338f, 0.0019317176192998886f, 0.002365788444876671f, 0.004379588644951582f, 0.0022388168144971132f, 0.003088494995608926f, 0.004742133431136608f, 0.002138732001185417f, 0.004964795894920826f, 0.0052364785224199295f, 0.0015304635744541883f, 0.0023424553219228983f, 0.005390726029872894f, 0.0032664721366018057f, 0.0030745251569896936f, 0.004017229191958904f, 0.007335521746426821f, 0.0032141695264726877f, 0.004240510053932667f, 0.004980519879609346f, 0.007198361214250326f, 0.00559799000620842f, 0.0034185214899480343f, 0.005311544518917799f, 0.0066615138202905655f, 0.006384204141795635f, 0.007350604049861431f, 0.00739316176623106f, 0.0058702025562524796f, 0.0012972670374438167f, 0.0051840548403561115f, 0.0020523692946881056f, 0.0062704021111130714f, 0.00498291477560997f, 0.0025914236903190613f, 0.003068754216656089f, 0.002872304990887642f, 0.004536137916147709f, 0.00393629539757967f, 0.0024003421422094107f, 0.008416000753641129f, 0.0050943950191140175f, 0.0034836740233004093f, 0.02000916562974453f, 0.006724242120981216f, 0.002763159107416868f, 0.00559089845046401f, 0.004729419015347958f, 0.0050531174056231976f, 0.004724029451608658f);
static const ai_layer_format_type conv2d_8_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_9_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_9_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_9_pad_before_t_in_0_shape_h_const_u32 = 28;

static const ai_u16 conv2d_9_t_in_0_shape_w_const_u16 = 30;
static const ai_u16 conv2d_9_t_in_0_shape_h_const_u16 = 30;
static const ai_u16 conv2d_9_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_9_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_9_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_9_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_9_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_9_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_9_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_9_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.020071743056178093f, 0.019144630059599876f, 0.00741869630292058f, 0.007954662665724754f, 0.019811594858765602f, 0.020783554762601852f, 0.009205606766045094f, 0.003813062096014619f, 0.013495358638465405f, 0.01447396818548441f, 0.012562410905957222f, 0.019619546830654144f, 0.007506529334932566f, 0.010814829729497433f, 0.012655513361096382f, 0.01206063199788332f, 0.005035138688981533f, 0.006056769285351038f, 0.0043663522228598595f, 0.012037631124258041f, 0.008053152821958065f, 0.008426251821219921f, 0.007822838611900806f, 0.010299542918801308f, 0.016728373244404793f, 0.011251338757574558f, 0.01064340490847826f, 0.007606807164847851f, 0.004883165936917067f, 0.011588730849325657f, 0.034654147922992706f, 0.00838278979063034f, 0.009147681295871735f, 0.012554263696074486f, 0.0079234903678298f, 0.0329342857003212f, 0.0037788087502121925f, 0.0038574093487113714f, 0.008036940358579159f, 0.015270031057298183f, 0.009803381748497486f, 0.008298531174659729f, 0.013359304517507553f, 0.016650613397359848f, 0.02093249000608921f, 0.00378230563364923f, 0.0073181623592972755f, 0.014395499601960182f, 0.00977085530757904f, 0.006021460983902216f, 0.02306962199509144f, 0.01027237344533205f, 0.006920758169144392f, 0.016392089426517487f, 0.013039699755609035f, 0.0028206249698996544f, 0.00567303691059351f, 0.009092243388295174f, 0.02241538278758526f, 0.00683578010648489f, 0.005833419971168041f, 0.012426295317709446f, 0.004636898171156645f, 0.002327123424038291f, 0.018048876896500587f, 0.012236813083291054f, 0.013561722822487354f, 0.009568039327859879f, 0.015135922469198704f, 0.007198607549071312f, 0.007199244573712349f, 0.015612006187438965f, 0.005866773892194033f, 0.009228198789060116f, 0.009432022459805012f, 0.014629385434091091f, 0.013096303679049015f, 0.029174072667956352f, 0.02332318387925625f, 0.012392688542604446f, 0.006286259274929762f, 0.007856018841266632f, 0.015954500064253807f, 0.011366967111825943f, 0.011240547522902489f, 0.016247378662228584f, 0.002993811620399356f, 0.003435299964621663f, 0.022785091772675514f, 0.009694099426269531f, 0.0032744852360337973f, 0.008725869469344616f, 0.007511918433010578f, 0.01082554366439581f, 0.006971430033445358f, 0.016901297494769096f, 0.009832119569182396f, 0.006370546296238899f, 0.010977084748446941f, 0.014657540246844292f, 0.011071340180933475f, 0.008790754713118076f, 0.004618069157004356f, 0.007431416306644678f, 0.00843312032520771f, 0.008128616027534008f, 0.0031155964825302362f, 0.02309911884367466f, 0.003700132016092539f, 0.015849394723773003f, 0.004136563278734684f, 0.0017581450520083308f, 0.00919344276189804f, 0.008994563482701778f, 0.009154334664344788f, 0.011880765669047832f, 0.014429199509322643f, 0.009083879180252552f, 0.0064969733357429504f, 0.016493026167154312f, 0.009064574725925922f, 0.008602618239820004f, 0.004864542279392481f, 0.010351554490625858f, 0.01403750665485859f, 0.00858917273581028f, 0.006961448583751917f, 0.013260909356176853f);
static const ai_u16 conv2d_9_t_out_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_9_t_out_0_shape_h_const_u16 = 28;

static const ai_u16 conv2d_10_t_in_0_shape_w_const_u16 = 28;
static const ai_u16 conv2d_10_t_in_0_shape_h_const_u16 = 28;
static const ai_u16 conv2d_10_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_10_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_10_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_10_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_10_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_10_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_10_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_10_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_10_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0024828959722071886f, 0.0028629431035369635f, 0.0026676133275032043f, 0.003155497834086418f, 0.003283130470663309f, 0.005398860201239586f, 0.006675401236861944f, 0.00417316472157836f, 0.005799922160804272f, 0.0037001839373260736f, 0.0049484772607684135f, 0.006899441592395306f, 0.004780804738402367f, 0.008419317193329334f, 0.002759540919214487f, 0.0026111442130059004f, 0.0038165899459272623f, 0.0025883300695568323f, 0.005410444922745228f, 0.007189363706856966f, 0.004061541054397821f, 0.0027222512289881706f, 0.0034957495518028736f, 0.0038011898286640644f, 0.003908172249794006f, 0.0034266705624759197f, 0.009087162092328072f, 0.0025941901840269566f, 0.0033021613489836454f, 0.00374886323697865f, 0.0030995658598840237f, 0.005686815362423658f, 0.004052094649523497f, 0.003894162829965353f, 8.566850738134235e-05f, 0.002733748871833086f, 0.0017151075880974531f, 0.004373259376734495f, 0.001232336857356131f, 0.006663774140179157f, 0.0014853831380605698f, 0.005870401859283447f, 0.003292182693257928f, 0.001138566411100328f, 0.0052468013018369675f, 0.003300665644928813f, 0.004033029079437256f, 0.001915972912684083f, 0.0032819872722029686f, 0.0073036449030041695f, 0.002101501217111945f, 0.0015240820357576013f, 0.006559405941516161f, 0.0043769399635493755f, 0.002995725255459547f, 0.004818211309611797f, 0.0014454112388193607f, 0.0015711578307673335f, 0.001878339797258377f, 0.00945253949612379f, 0.003240399295464158f, 0.003403765382245183f, 0.004715668503195047f, 0.002480998868122697f, 0.00536627322435379f, 0.0020841541700065136f, 0.0037639448419213295f, 0.007359038572758436f, 0.014454196207225323f, 0.002009096322581172f, 0.020133361220359802f, 0.0018918641144409776f, 0.0046138339675962925f, 0.0031843027099967003f, 0.004547171760350466f, 0.006449612323194742f, 0.0041709234938025475f, 0.0058813551440835f, 0.003091862890869379f, 0.0057769096456468105f, 0.0036882597487419844f, 0.005324470344930887f, 0.0045541697181761265f, 0.006766737438738346f, 0.003062334144487977f, 0.0067175705917179585f, 0.00474952720105648f, 0.0057756733149290085f, 0.003239885438233614f, 0.0014842802193015814f, 0.0044402386993169785f, 0.0029606609605252743f, 0.010167798958718777f, 0.0052257440984249115f, 0.0035717396531254053f, 0.011044842191040516f, 0.00446910597383976f, 0.008426113985478878f, 0.012746754102408886f, 0.0057090879417955875f, 0.003512816270813346f, 0.0041604191064834595f, 0.003977783024311066f, 0.0035576634109020233f, 0.010896659456193447f, 0.00190924690105021f, 0.0017490740865468979f, 0.003393782302737236f, 0.0032845952082425356f, 0.009864008985459805f, 0.0026682857424020767f, 0.0063433339819312096f, 0.006860883440822363f, 0.005021171644330025f, 0.0075157745741307735f, 0.0033649695105850697f, 0.0028102872893214226f, 0.0030997348949313164f, 0.0027461962308734655f, 0.002164167119190097f, 0.0042557804845273495f, 0.0032573386561125517f, 0.0017937044613063335f, 0.004974902141839266f, 0.002508665667846799f, 0.005089991260319948f, 0.0034521648194640875f, 0.0031098148319870234f);
static const ai_layer_format_type conv2d_10_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_11_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_11_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_11_pad_before_t_in_0_shape_h_const_u32 = 28;

static const ai_u16 conv2d_11_t_in_0_shape_w_const_u16 = 30;
static const ai_u16 conv2d_11_t_in_0_shape_h_const_u16 = 30;
static const ai_u16 conv2d_11_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_11_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_11_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_11_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_11_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_11_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_11_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_11_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004285551607608795f, 0.003891316242516041f, 0.007063320837914944f, 0.004597807768732309f, 0.0035507490392774343f, 0.0018930371152237058f, 0.002293023280799389f, 0.0036694456357508898f, 0.0026869061402976513f, 0.0034970196429640055f, 0.0029515926726162434f, 0.00394399743527174f, 0.0019850677344948053f, 0.0034537422470748425f, 0.003501532832160592f, 0.002342101652175188f, 0.0043350388295948505f, 0.0037882516626268625f, 0.0024060786236077547f, 0.0015271713491529226f, 0.0026090361643582582f, 0.0035107561852782965f, 0.0033407395239919424f, 0.0036035424564033747f, 0.0018853865331038833f, 0.0031180328223854303f, 0.0026638482231646776f, 0.004259506706148386f, 0.0036356355994939804f, 0.0029912993777543306f, 0.003389718011021614f, 0.004537414759397507f, 0.0052766259759664536f, 0.003287132130935788f, 0.09912855923175812f, 0.0037170981522649527f, 0.006817190907895565f, 0.004118285607546568f, 0.010962407104671001f, 0.002881024731323123f, 0.00900263898074627f, 0.004953999537974596f, 0.00439427001401782f, 0.006493999622762203f, 0.004023826215416193f, 0.003695243503898382f, 0.006549687124788761f, 0.00688255624845624f, 0.004184133838862181f, 0.0018922144081443548f, 0.0038830838166177273f, 0.0029255126137286425f, 0.003974131308495998f, 0.002889910014346242f, 0.007651473395526409f, 0.0034661684185266495f, 0.004278498701751232f, 0.011060845106840134f, 0.006233231630176306f, 0.002673179842531681f, 0.0039352611638605595f, 0.0027866384480148554f, 0.0041179293766617775f, 0.0034131468273699284f, 0.003626396879553795f, 0.005133132915943861f, 0.0033469076734036207f, 0.0020551958587020636f, 0.0017988835461437702f, 0.010180313140153885f, 0.003344368189573288f, 0.0044199563562870026f, 0.0030453656800091267f, 0.005111880600452423f, 0.0035939388908445835f, 0.002774494932964444f, 0.0038104935083538294f, 0.003281119978055358f, 0.0039013673085719347f, 0.00392613559961319f, 0.005373590160161257f, 0.003366412129253149f, 0.0025110167916864157f, 0.0021109210792928934f, 0.004643948283046484f, 0.002763323485851288f, 0.004079355858266354f, 0.0031801366712898016f, 0.003851943649351597f, 0.005164149682968855f, 0.0035230733919888735f, 0.004002220928668976f, 0.002355879172682762f, 0.007613632827997208f, 0.003413420170545578f, 0.0018069271463900805f, 0.003735271282494068f, 0.0017152370419353247f, 0.002099603181704879f, 0.001666452968493104f, 0.003466066438704729f, 0.0029928344301879406f, 0.006381777580827475f, 0.004124523140490055f, 0.002152827102690935f, 0.0037639171350747347f, 0.00575973279774189f, 0.002902230015024543f, 0.0036874262150377035f, 0.002607570495456457f, 0.003953279461711645f, 0.0012790540931746364f, 0.0020014995243400335f, 0.0026125528384000063f, 0.0013332704547792673f, 0.003705682000145316f, 0.004146452061831951f, 0.003856991184875369f, 0.001978885615244508f, 0.00380714307539165f, 0.002539938548579812f, 0.003403580514714122f, 0.004301778506487608f, 0.0039288257248699665f, 0.00487839849665761f, 0.002482253359630704f, 0.003711537690833211f, 0.002951010363176465f);
static const ai_u16 conv2d_11_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_11_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_12_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_12_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_12_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_12_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_12_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_12_t_out_0_shape_ch_const_u16 = 256;
static const ai_i8 conv2d_12_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_12_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_12_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_12_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_12_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.003971569240093231f, 0.0029973566997796297f, 0.0034633479081094265f, 0.006653291173279285f, 0.005117721389979124f, 0.005279315169900656f, 0.0022190609015524387f, 0.0033364221453666687f, 0.0017559308325871825f, 0.0026374522130936384f, 0.003259121673181653f, 0.005903373006731272f, 0.001993811223655939f, 0.004714353010058403f, 0.0026377416215837f, 0.0037281885743141174f, 0.0036263479851186275f, 0.0062440368346869946f, 0.003203131491318345f, 0.002261318499222398f, 0.0037914528511464596f, 0.0015151746338233352f, 0.0034048594534397125f, 0.003921115770936012f, 0.0022969257552176714f, 0.0020807015243917704f, 0.002701948396861553f, 0.005991389974951744f, 0.0035849541891366243f, 0.005198240280151367f, 0.004063024651259184f, 0.004795505199581385f, 0.0036810068413615227f, 0.0031614506151527166f, 0.005624962970614433f, 0.009465656243264675f, 0.005057547241449356f, 0.003318342613056302f, 0.004022215958684683f, 0.003992555662989616f, 0.0028336760587990284f, 0.004312867298722267f, 0.0025728459004312754f, 0.002410393673926592f, 0.004891406744718552f, 0.0033718023914843798f, 0.003931667190045118f, 0.0013520290376618505f, 0.0019895480945706367f, 0.0015184162184596062f, 0.0037184590473771095f, 0.0029789262916892767f, 0.004923026077449322f, 0.005344056989997625f, 0.003023256314918399f, 0.0023707663640379906f, 0.004424817860126495f, 0.004145007114857435f, 0.0033231324050575495f, 0.0025380670558661222f, 0.0026379998307675123f, 0.0032514517661184072f, 0.003199163591489196f, 0.006996780168265104f, 0.0019115095492452383f, 0.001943513983860612f, 0.007276399061083794f, 0.0012310963356867433f, 0.0015871976502239704f, 0.0026879163924604654f, 0.0031077919993549585f, 0.0023314598947763443f, 0.0013090332504361868f, 0.002322586951777339f, 0.011580330319702625f, 0.005552354268729687f, 0.003358629997819662f, 0.0027519455179572105f, 0.00275023584254086f, 0.0030137598514556885f, 0.0029189588967710733f, 0.01532271783798933f, 0.003590279957279563f, 0.0031057416927069426f, 0.002891112118959427f, 0.0032532894983887672f, 0.0018156780861318111f, 0.0027367747388780117f, 0.002785500604659319f, 0.002392095047980547f, 0.004771020263433456f, 0.0016835381975397468f, 0.007689152378588915f, 0.004431271925568581f, 0.004999767057597637f, 0.0012910643126815557f, 0.002393921837210655f, 0.0028364849276840687f, 0.0033450149931013584f, 0.002433758694678545f, 0.0036024851724505424f, 0.004090311471372843f, 0.0019763908348977566f, 0.004508599638938904f, 0.0014132297364994884f, 0.0038435393944382668f, 0.0032891607843339443f, 0.004165275953710079f, 0.003860010067000985f, 0.002540991175919771f, 0.0038012005388736725f, 0.0040755486115813255f, 0.006180936004966497f, 0.003451173659414053f, 0.004109944682568312f, 0.0032759560272097588f, 0.0034527366515249014f, 0.003345799632370472f, 0.011750031262636185f, 0.003180207684636116f, 0.005107142496854067f, 0.0041633774526417255f, 0.003048151498660445f, 0.0034841354936361313f, 0.0029512157198041677f, 0.00306437024846673f, 0.0031219031661748886f, 0.0022854856215417385f, 0.003189811483025551f, 0.001889758394099772f, 0.002006866969168186f, 0.002994236070662737f, 0.00405065668746829f, 0.0019693479407578707f, 0.002782905474305153f, 0.0015690061263740063f, 0.002000609179958701f, 0.004247482866048813f, 0.003741556080058217f, 0.005546258762478828f, 0.004943607375025749f, 0.002214123262092471f, 0.0028434423729777336f, 0.00417523505166173f, 0.0032110754400491714f, 0.004639385733753443f, 0.003527572611346841f, 0.0047447578981518745f, 0.005257489625364542f, 0.006409290246665478f, 0.0026749561075121164f, 0.0022194879129529f, 0.0019020154140889645f, 0.004223437048494816f, 0.003793847979977727f, 0.004287692252546549f, 0.002958401571959257f, 0.002925351494923234f, 0.002682490274310112f, 0.001862707664258778f, 0.0028135129250586033f, 0.0028161366935819387f, 0.005827877204865217f, 0.0037841203156858683f, 0.0027893492951989174f, 0.0017112806672230363f, 0.002480250084772706f, 0.0023620103020220995f, 0.003363453783094883f, 0.002441460732370615f, 0.0031538126058876514f, 0.0015616208547726274f, 0.003239922458305955f, 0.0009553142008371651f, 0.010038254782557487f, 0.0016907162498682737f, 0.004823417402803898f, 0.003970743156969547f, 0.0049909367226064205f, 0.001825051149353385f, 0.004078744910657406f, 0.0021681503858417273f, 0.0035609337501227856f, 0.003220794489607215f, 0.00929754227399826f, 0.005437720566987991f, 0.0012236490147188306f, 0.0021303235553205013f, 0.002116808434948325f, 0.0024411368649452925f, 0.002102848142385483f, 0.0023808134719729424f, 0.003323717275634408f, 0.003208046779036522f, 0.0013494726736098528f, 0.003238003933802247f, 0.001760573242790997f, 0.00316651351749897f, 0.0028366120532155037f, 0.0017833568854257464f, 0.0038584673311561346f, 0.0053168004378676414f, 0.0022363720927387476f, 0.01157120056450367f, 0.0013899445766583085f, 0.006530071143060923f, 0.003140328684821725f, 0.0013044654624536633f, 0.0033844895660877228f, 0.0026933408807963133f, 0.003706041956320405f, 0.002289010211825371f, 0.0072991447523236275f, 0.0035508088767528534f, 0.0023308107629418373f, 0.0030144397169351578f, 0.0020449880976229906f, 0.002291302429512143f, 0.001726620364934206f, 0.003753734054043889f, 0.0022722159046679735f, 0.0015343973645940423f, 0.0023504795972257853f, 0.0025368251372128725f, 0.004034331534057856f, 0.0038561467081308365f, 0.0022477121092379093f, 0.010634463280439377f, 0.0032347424421459436f, 0.006187289021909237f, 0.0033378279767930508f, 0.00825065653771162f, 0.002043510554358363f, 0.002817430766299367f, 0.00315823289565742f, 0.002279886044561863f, 0.0019413791596889496f, 0.005465564783662558f, 0.0020214945543557405f, 0.0022543948143720627f, 0.003929259721189737f, 0.0048110755160450935f, 0.004920596722513437f, 0.003715748433023691f, 0.0028137092012912035f, 0.002958102384582162f, 0.004185711033642292f, 0.002903911517933011f, 0.024244483560323715f, 0.0030720194336026907f, 0.003441150998696685f, 0.005311458837240934f, 0.0020093799103051424f, 0.0023431298322975636f, 0.0011833450989797711f, 0.0014115386875346303f);
static const ai_layer_format_type conv2d_12_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_13_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_13_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_13_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_13_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_13_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_13_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_13_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_13_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_13_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_13_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_13_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_13_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_13_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.011532901786267757f, 0.008802340365946293f, 0.006611029617488384f, 0.006658510770648718f, 0.00642845593392849f, 0.007327357307076454f, 0.009428114630281925f, 0.009798686020076275f, 0.01088725682348013f, 0.015375223942101002f, 0.0072127217426896095f, 0.010026682168245316f, 0.009471772238612175f, 0.011968026868999004f, 0.0038635265082120895f, 0.01042777020484209f, 0.008439523167908192f, 0.008708799257874489f, 0.01024326216429472f, 0.004694722592830658f, 0.010025604628026485f, 0.015002674423158169f, 0.008992334827780724f, 0.006545847747474909f, 0.009519259445369244f, 0.01264503039419651f, 0.00880312081426382f, 0.005364499054849148f, 0.010030045174062252f, 0.008774436078965664f, 0.010356507264077663f, 0.007612648885697126f, 0.01272291224449873f, 0.009564576670527458f, 0.007797724101692438f, 0.006732359528541565f, 0.007723743561655283f, 0.00885175820440054f, 0.0060383169911801815f, 0.00887396652251482f, 0.00851390603929758f, 0.00929680373519659f, 0.014603067189455032f, 0.010197358205914497f, 0.0037196879275143147f, 0.00939919613301754f, 0.013442880474030972f, 0.02056511864066124f, 0.019351864233613014f, 0.012918876484036446f, 0.013194078579545021f, 0.010336039587855339f, 0.0012534172274172306f, 0.00970382895320654f, 0.012420146726071835f, 0.01026181224733591f, 0.002623011125251651f, 0.008816834539175034f, 0.01236772071570158f, 0.013637830503284931f, 0.014994846656918526f, 0.011596991680562496f, 0.007946710102260113f, 0.010204996913671494f, 0.017462465912103653f, 0.010758264921605587f, 0.008704042993485928f, 0.010232623666524887f, 0.01634969376027584f, 0.008040419779717922f, 0.011301398277282715f, 0.018782680854201317f, 0.01642206683754921f, 0.009323407895863056f, 0.0038098637014627457f, 0.009443473070859909f, 0.014091414399445057f, 0.006327430717647076f, 0.010171079076826572f, 0.020346583798527718f, 0.01636083610355854f, 0.0013381101889535785f, 0.010561746545135975f, 0.015236292034387589f, 0.004180729389190674f, 0.008473658934235573f, 0.011678891256451607f, 0.010903355665504932f, 0.01179872453212738f, 0.010425782762467861f, 0.00939934141933918f, 0.013441454619169235f, 0.0032420787028968334f, 0.007251739967614412f, 0.009927187114953995f, 0.024944793432950974f, 0.013278605416417122f, 0.01764015108346939f, 0.0060370261780917645f, 0.01280065905302763f, 0.009546003304421902f, 0.0075025400146842f, 0.0056125931441783905f, 0.007437299005687237f, 0.014624586328864098f, 0.012194820679724216f, 0.007563418708741665f, 0.012294710613787174f, 0.001181614468805492f, 0.014553114771842957f, 0.00553520442917943f, 0.004737214185297489f, 0.0051679364405572414f, 0.010963200591504574f, 0.008689832873642445f, 0.007641712669283152f, 0.013005363754928112f, 0.00704929418861866f, 0.001694673323072493f, 0.010420356877148151f, 0.012363879941403866f, 0.005615083500742912f, 0.010168228298425674f, 0.0086180055513978f, 0.009865762665867805f, 0.0098477303981781f, 0.007825770415365696f, 0.011773524805903435f, 0.011591090820729733f, 0.013235023245215416f, 0.011189519427716732f, 0.007474158890545368f, 0.012594528496265411f, 0.011237503960728645f, 0.010664030909538269f, 0.023302093148231506f, 0.011644873768091202f, 0.00896964780986309f, 0.007612729445099831f, 0.02137228287756443f, 0.011186129413545132f, 0.007655859924852848f, 0.011631404981017113f, 0.013805273920297623f, 0.008465479128062725f, 0.004864168353378773f, 0.013432186096906662f, 0.005359130911529064f, 0.013638293370604515f, 0.0063176024705171585f, 0.013525336049497128f, 0.012269136495888233f, 0.012492142617702484f, 0.011494230479001999f, 0.006710042245686054f, 0.006512733176350594f, 0.006915690377354622f, 0.01204344630241394f, 0.008228316903114319f, 0.011640092357993126f, 0.012768248096108437f, 0.010946990922093391f, 0.009183683432638645f, 0.007750027347356081f, 0.010657628998160362f, 0.01869378425180912f, 0.00611691502854228f, 0.005971986800432205f, 0.008634770289063454f, 0.00795657653361559f, 0.012405679561197758f, 0.010137971490621567f, 0.017992019653320312f, 0.018881885334849358f, 0.001061108079738915f, 0.008695998229086399f, 0.008566228672862053f, 0.006235429551452398f, 0.007529209367930889f, 0.008390234783291817f, 0.011072386056184769f, 0.01426429022103548f, 0.011201600544154644f, 0.012800357304513454f, 0.0022448962554335594f, 0.010737081989645958f, 0.0116510521620512f, 0.007753212470561266f, 0.01577150635421276f, 0.007756162900477648f, 0.008759105578064919f, 0.010970475152134895f, 0.013968313112854958f, 0.012716790661215782f, 0.013497489504516125f, 0.014616000466048717f, 0.007543641608208418f, 0.01863374002277851f, 0.01113620214164257f, 0.008436379954218864f, 0.005667181685566902f, 0.005249470006674528f, 0.0066913217306137085f, 0.005437757354229689f, 0.016827160492539406f, 0.009925583377480507f, 0.011934375390410423f, 0.019255375489592552f, 0.010932667180895805f, 0.00558231957256794f, 0.00902209896594286f, 0.009447764605283737f, 0.004391870461404324f, 0.007253506686538458f, 0.012152843177318573f, 0.009731155820190907f, 0.009783436544239521f, 0.013048467226326466f, 0.011266079731285572f, 0.008569472469389439f, 0.01209944672882557f, 0.00845410954207182f, 0.009449826553463936f, 0.008419853635132313f, 0.005867804866284132f, 0.006464123725891113f, 0.015103098005056381f, 0.006067290436476469f, 0.0077443914487957954f, 0.003082038601860404f, 0.0064936005510389805f, 0.006356739439070225f, 0.003176460973918438f, 0.006565174553543329f, 0.003915066830813885f, 0.008106243796646595f, 0.011725572869181633f, 0.007298546843230724f, 0.015385422855615616f, 0.00972644705325365f, 0.010176113806664944f, 0.005543895065784454f, 0.0054897950030863285f, 0.007957672700285912f, 0.013479234650731087f, 0.013327405788004398f, 0.005276786629110575f, 0.008346827700734138f, 0.0016122462693601847f, 0.011675980873405933f, 0.006278974935412407f, 0.008772131986916065f, 0.017215054482221603f, 0.014898288063704967f, 0.015650054439902306f, 0.013619045726954937f);
static const ai_u16 conv2d_13_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_13_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_14_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_14_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_14_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_14_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_14_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_14_t_out_0_shape_ch_const_u16 = 256;
static const ai_i8 conv2d_14_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_14_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_14_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_14_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_14_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004147659987211227f, 0.002088081557303667f, 0.0019639814272522926f, 0.0028462898917496204f, 0.004934318829327822f, 0.0019457824528217316f, 0.0038275565020740032f, 0.0028424637857824564f, 0.003942646086215973f, 0.001864593243226409f, 0.0011153052328154445f, 0.001606240402907133f, 0.0012446197215467691f, 0.0026041404344141483f, 0.0042433482594788074f, 0.002645354252308607f, 0.001257171737961471f, 0.0027589525561779737f, 0.0021318544168025255f, 0.0006099998136050999f, 0.0018373413477092981f, 0.0017128189792856574f, 0.002312663709744811f, 0.004452915862202644f, 0.0012047745985910296f, 0.004089381080120802f, 0.0006923521286807954f, 0.0028406379278749228f, 0.0024840787518769503f, 0.00159009441267699f, 0.002631785813719034f, 0.0022041783668100834f, 0.0011936663649976254f, 0.0037607322447001934f, 0.0019529146375134587f, 0.0022343688178807497f, 0.0015225805109366775f, 0.00249445135705173f, 0.006372273433953524f, 0.002608907874673605f, 0.0015177695313468575f, 0.002087349072098732f, 0.0035986050497740507f, 0.0027650021947920322f, 0.00212760828435421f, 0.0018057775450870395f, 0.0012183610815554857f, 0.0012163626961410046f, 0.005002008285373449f, 0.002630470087751746f, 0.0036564269103109837f, 0.002740181051194668f, 0.004471708554774523f, 0.0024885686580091715f, 0.0008808228885754943f, 0.004578034393489361f, 0.001863852608948946f, 0.0011821856023743749f, 0.002017926424741745f, 0.0028500910848379135f, 0.002632184186950326f, 0.002753986045718193f, 0.002952681854367256f, 0.001217710436321795f, 0.0030106345657259226f, 0.0024871807545423508f, 0.0036808166187256575f, 0.002072752919048071f, 0.0030312007293105125f, 0.0021958816796541214f, 0.002579515567049384f, 0.0023424248211085796f, 0.0012988612288609147f, 0.0020662127062678337f, 0.001689962693490088f, 0.0014788889093324542f, 0.0024402018170803785f, 0.001949572004377842f, 0.0037803833838552237f, 0.0038906463887542486f, 0.002391363261267543f, 0.004111133050173521f, 0.0068915775045752525f, 0.0040843067690730095f, 0.00282044499181211f, 0.0015768706798553467f, 0.0023089114110916853f, 0.0025027345400303602f, 0.0020107135642319918f, 0.0025869570672512054f, 0.0015870090574026108f, 0.0013544128742069006f, 0.0036032753996551037f, 0.0008979174308478832f, 0.0035724493209272623f, 0.002219882095232606f, 0.005271904170513153f, 0.0014459274243563414f, 0.0044021583162248135f, 0.0020650047808885574f, 0.001041392213664949f, 0.0024257581681013107f, 0.0019089221023023129f, 0.0011174496030434966f, 0.0020330767147243023f, 0.0026511158794164658f, 0.001917528104968369f, 0.0025499053299427032f, 0.0030085628386586905f, 0.0026035928167402744f, 0.004410847090184689f, 0.0016062803333625197f, 0.002179475734010339f, 0.001172610092908144f, 0.0017337464960291982f, 0.0014845732366666198f, 0.0030210346449166536f, 0.005420641973614693f, 0.006455066613852978f, 0.0019225555006414652f, 0.0017963523278012872f, 0.002135937800630927f, 0.0005104101728647947f, 0.0021368595771491528f, 0.0020524361170828342f, 0.0033780515659600496f, 0.005646440200507641f, 0.002409678651019931f, 0.0032140854746103287f, 0.0036351564340293407f, 0.002885195193812251f, 0.0022481216583400965f, 0.003274525050073862f, 0.0019150720909237862f, 0.0032616821117699146f, 0.002688118489459157f, 0.002706375205889344f, 0.0018123482586815953f, 0.0018476383993402123f, 0.0018760432722046971f, 0.0034944836515933275f, 0.003745799418538809f, 0.004070531111210585f, 0.003455960424616933f, 0.006988534703850746f, 0.004469549283385277f, 0.005648184102028608f, 0.0013992483727633953f, 0.0025649371091276407f, 0.001744484412483871f, 0.003096302505582571f, 0.003368639387190342f, 0.0019800688605755568f, 0.0021143434569239616f, 0.005554544273763895f, 0.004510534927248955f, 0.0026198639534413815f, 0.002785605378448963f, 0.0017962587298825383f, 0.002011490985751152f, 0.001907413243316114f, 0.0017289267852902412f, 0.0031829862855374813f, 0.0016186356078833342f, 0.0018040258437395096f, 0.0034326098393648863f, 0.0029164040461182594f, 0.0019614039920270443f, 0.002690229332074523f, 0.004261255729943514f, 0.0015133674023672938f, 0.003689579898491502f, 0.0028650397434830666f, 0.0032722086180001497f, 0.0025249647442251444f, 0.002162360120564699f, 0.0035597076639533043f, 0.003953065723180771f, 0.0016565527766942978f, 0.0017646076157689095f, 0.004264798481017351f, 0.002649292815476656f, 0.0019955409225076437f, 0.0025087492540478706f, 0.005544002167880535f, 0.002934488235041499f, 0.005877775140106678f, 0.0051558674313127995f, 0.0008655434940010309f, 0.00560797331854701f, 0.0022712720092386007f, 0.006162097677588463f, 0.0013264140579849482f, 0.005171656608581543f, 0.002017489867284894f, 0.002611394738778472f, 0.0018449948402121663f, 0.004646885208785534f, 0.001999438274651766f, 0.003846652340143919f, 0.0017613451927900314f, 0.0026851282455027103f, 0.001647583907470107f, 0.002600582782179117f, 0.0030632170382887125f, 0.00511102145537734f, 0.003969388548284769f, 0.0015740635572001338f, 0.0026142147835344076f, 0.003804480191320181f, 0.004393308889120817f, 0.0022114377934485674f, 0.004063337575644255f, 0.00208594580180943f, 0.0018940597074106336f, 0.0018395206425338984f, 0.0026189035270363092f, 0.0027376359794288874f, 0.003553403541445732f, 0.0014376189792528749f, 0.002044757828116417f, 0.00406243372708559f, 0.0012155472068116069f, 0.006396159064024687f, 0.0027644908986985683f, 0.0012214604066684842f, 0.0018070833757519722f, 0.00300219957716763f, 0.0033986824564635754f, 0.01284017413854599f, 0.0030582610052078962f, 0.004855822306126356f, 0.0019918058533221483f, 0.0012088982621207833f, 0.003988625016063452f, 0.0005786237306892872f, 0.003921452444046736f, 0.002882589353248477f, 0.00321973767131567f, 0.0021401597186923027f, 0.0011436770437285304f, 0.002203657990321517f, 0.0020159417763352394f, 0.0040000188164412975f, 0.005211845505982637f, 0.002147262217476964f, 0.0030955716501921415f, 0.0021106498315930367f, 0.002654594834893942f, 0.003091648919507861f, 0.0038141522090882063f, 0.003920030314475298f, 0.0017181216971948743f, 0.002869396936148405f, 0.00220245448872447f, 0.0038815937004983425f);
static const ai_layer_format_type conv2d_14_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_15_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_15_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_15_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_15_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_15_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_15_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_15_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_15_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_15_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_15_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_15_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_15_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_15_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.007112117018550634f, 0.009550828486680984f, 0.01366366259753704f, 0.004810546990483999f, 0.0077787116169929504f, 0.008306452073156834f, 0.008417598903179169f, 0.004778754431754351f, 0.003142771776765585f, 0.015567024238407612f, 0.016440702602267265f, 0.010093960911035538f, 0.013682120479643345f, 0.010843219235539436f, 0.008053491823375225f, 0.008829576894640923f, 0.011689605191349983f, 0.0037649457808583975f, 0.007765615824609995f, 0.03085365891456604f, 0.008374757133424282f, 0.037328191101551056f, 0.008996191434562206f, 0.005107112228870392f, 0.017107296735048294f, 0.006760409567505121f, 0.017396360635757446f, 0.007013046648353338f, 0.004780460149049759f, 0.008488423191010952f, 0.008310774341225624f, 0.008352273143827915f, 0.010725943371653557f, 0.00495290569961071f, 0.008605743758380413f, 0.009426666423678398f, 0.0073661948554217815f, 0.0023153393995016813f, 0.006810707505792379f, 0.0051994905807077885f, 0.008644817396998405f, 0.006832926068454981f, 0.008040042594075203f, 0.0032927729189395905f, 0.011255809105932713f, 0.014282049611210823f, 0.014433366246521473f, 0.014129409566521645f, 0.003085552481934428f, 0.008538275957107544f, 0.007933806627988815f, 0.00431135855615139f, 0.0026463218964636326f, 0.002759890863671899f, 0.00888397078961134f, 0.0016691306373104453f, 0.010438626632094383f, 0.010519738309085369f, 0.007769704796373844f, 0.005064491648226976f, 0.0027556363493204117f, 0.0024384604766964912f, 0.01457113865762949f, 0.011697121895849705f, 0.005069469567388296f, 0.00973653607070446f, 0.003873205743730068f, 0.009099560789763927f, 0.004897078033536673f, 0.007141808979213238f, 0.006890004500746727f, 0.011693972162902355f, 0.012535011395812035f, 0.008530283346772194f, 0.015198802575469017f, 0.0034100685734301805f, 0.007254465017467737f, 0.017712045460939407f, 0.0015757991932332516f, 0.006092479452490807f, 0.0057537429966032505f, 0.004254654049873352f, 0.0031979584600776434f, 0.00285134743899107f, 0.0055959331803023815f, 0.01336103118956089f, 0.005036984570324421f, 0.009779940359294415f, 0.006696356926113367f, 0.005575149320065975f, 0.009674099273979664f, 0.0036262408830225468f, 0.006917136255651712f, 0.016292721033096313f, 0.0018476074328646064f, 0.0040674833580851555f, 0.0016717209946364164f, 0.011585969477891922f, 0.008269726298749447f, 0.004573810379952192f, 0.01798815093934536f, 0.012407572939991951f, 0.0038308657240122557f, 0.023092005401849747f, 0.010385359637439251f, 0.003524953033775091f, 0.008518833667039871f, 0.005929887294769287f, 0.005655261222273111f, 0.011451763100922108f, 0.007492275908589363f, 0.011984976008534431f, 0.011706467717885971f, 0.01317170076072216f, 0.005928893573582172f, 0.015483984723687172f, 0.0036245626397430897f, 0.007566545624285936f, 0.0044748722575604916f, 0.01324137207120657f, 0.004722768906503916f, 0.0050174398347735405f, 0.03165018931031227f, 0.010921058245003223f, 0.01058395765721798f, 0.006195724010467529f, 0.003837771015241742f, 0.00572990532964468f, 0.006690110079944134f, 0.006894328165799379f, 0.008623212575912476f, 0.003757548751309514f, 0.008064703084528446f, 0.008105532266199589f, 0.0034162099473178387f, 0.010047314688563347f, 0.003856089198961854f, 0.005299414042383432f, 0.004404786508530378f, 0.005416778847575188f, 0.007941854186356068f, 0.011561281979084015f, 0.0022690300829708576f, 0.0064253369346261024f, 0.003472592681646347f, 0.006854069419205189f, 0.0025888695381581783f, 0.00990556925535202f, 0.007991031743586063f, 0.011533187702298164f, 0.004191225860267878f, 0.0034237203653901815f, 0.0064242067746818066f, 0.005012198816984892f, 0.006928993854671717f, 0.002559797838330269f, 0.009167094714939594f, 0.0032409140840172768f, 0.012431656010448933f, 0.009014778770506382f, 0.004549358505755663f, 0.011921650730073452f, 0.006799828726798296f, 0.012352647259831429f, 0.009206753224134445f, 0.006954023148864508f, 0.007328138221055269f, 0.009102903306484222f, 0.0074751367792487144f, 0.005297255236655474f, 0.012138724327087402f, 0.006763098295778036f, 0.007901392877101898f, 0.002513305516913533f, 0.008927841670811176f, 0.007401742041110992f, 0.003167046932503581f, 0.007242241408675909f, 0.00756579777225852f, 0.007254411466419697f, 0.006327103357762098f, 0.007388176862150431f, 0.012072739191353321f, 0.005202750209718943f, 0.007132791914045811f, 0.011455049738287926f, 0.0038948969449847937f, 0.002009051153436303f, 0.013892898336052895f, 0.006757303141057491f, 0.009374729357659817f, 0.0069118388928473f, 0.012371723540127277f, 0.0022290365304797888f, 0.012515698559582233f, 0.005630853585898876f, 0.008752166293561459f, 0.00733304675668478f, 0.007193410769104958f, 0.006630509626120329f, 0.007116447202861309f, 0.011523470282554626f, 0.01501361932605505f, 0.019606972113251686f, 0.0019219279056414962f, 0.0028777732513844967f, 0.002526629948988557f, 0.008738608099520206f, 0.005118203349411488f, 0.008140305057168007f, 0.010158375836908817f, 0.0056481692008674145f, 0.002154533751308918f, 0.005091710016131401f, 0.005331528373062611f, 0.006381406914442778f, 0.010151312686502934f, 0.0051361387595534325f, 0.008494790643453598f, 0.012905541807413101f, 0.018029451370239258f, 0.005090116988867521f, 0.0076560997404158115f, 0.0049934689886868f, 0.008284245617687702f, 0.008694864809513092f, 0.012354342266917229f, 0.0018925985787063837f, 0.007549632340669632f, 0.002871939679607749f, 0.00488077849149704f, 0.006627112161368132f, 0.006052129436284304f, 0.01234077475965023f, 0.003348540049046278f, 0.022723348811268806f, 0.00434737466275692f, 0.003211074275895953f, 0.0023376764729619026f, 0.002119921613484621f, 0.014831215143203735f, 0.006251802667975426f, 0.006228301208466291f, 0.002318131970241666f, 0.0020508114248514175f, 0.00787605531513691f, 0.007934372872114182f, 0.0075470334850251675f, 0.013254517689347267f, 0.0077523160725831985f, 0.003804250620305538f, 0.0020072374027222395f, 0.00829294603317976f, 0.0052681853994727135f, 0.0067683374509215355f, 0.007153037004172802f);
static const ai_u16 conv2d_15_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_15_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_16_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_16_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_16_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_16_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_16_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_16_t_out_0_shape_ch_const_u16 = 256;
static const ai_i8 conv2d_16_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_16_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_16_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_16_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_16_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002921489765867591f, 0.0017243260517716408f, 0.0015099148731678724f, 0.001807652646675706f, 0.0033787714783102274f, 0.002397384960204363f, 0.0017716301372274756f, 0.0026093998458236456f, 0.0032483127433806658f, 0.0022434052079916f, 0.0033834888599812984f, 0.0025782191660255194f, 0.0035640555433928967f, 0.003126197261735797f, 0.0030886942986398935f, 0.004032728262245655f, 0.0023752048145979643f, 0.002150272950530052f, 0.001793051022104919f, 0.0016362317837774754f, 0.003603652585297823f, 0.0021232282742857933f, 0.0032272273674607277f, 0.0030505394097417593f, 0.00521282060071826f, 0.0020844542887061834f, 0.0027960515581071377f, 0.005714038386940956f, 0.0024638702161610126f, 0.00231790728867054f, 0.0032247197814285755f, 0.002663098741322756f, 0.002336333505809307f, 0.0012478138087317348f, 0.0038344559725373983f, 0.003905988298356533f, 0.0025737399701029062f, 0.001099662738852203f, 0.0022960773203521967f, 0.003869779175147414f, 0.0035519080702215433f, 0.0012597099412232637f, 0.002992150839418173f, 0.003060411661863327f, 0.005067326128482819f, 0.005453058518469334f, 0.0020130095072090626f, 0.006250015925616026f, 0.002169610233977437f, 0.00165372749324888f, 0.003006427315995097f, 0.005464436952024698f, 0.002229769481346011f, 0.005059783812612295f, 0.0030564172193408012f, 0.00516615342348814f, 0.0025773122906684875f, 0.005035030655562878f, 0.0021641147322952747f, 0.0033627727534621954f, 0.006673119496554136f, 0.0024483518209308386f, 0.0029718803707510233f, 0.004063889384269714f, 0.002830076962709427f, 0.005980407353490591f, 0.001958529930561781f, 0.0027986716013401747f, 0.002680934267118573f, 0.0028034551069140434f, 0.0026205407921224833f, 0.0033435055520385504f, 0.003371400758624077f, 0.0035694497637450695f, 0.0023178216069936752f, 0.003325704950839281f, 0.0030596565920859575f, 0.002808039542287588f, 0.0029223805759102106f, 0.0033334384206682444f, 0.0040753073990345f, 0.002733878092840314f, 0.0026446471456438303f, 0.0012183253420516849f, 0.0028045305516570807f, 0.0032567193266004324f, 0.0018285041442140937f, 0.0019624463748186827f, 0.0027684092056006193f, 0.00317406072281301f, 0.001955477986484766f, 0.0017596859252080321f, 0.0012081883614882827f, 0.002433784306049347f, 0.002426888793706894f, 0.0037832395173609257f, 0.001811556052416563f, 0.003334887558594346f, 0.0017668032087385654f, 0.002329056616872549f, 0.005052933935075998f, 0.0034663209225982428f, 0.003428874770179391f, 0.0019470127299427986f, 0.0034639283549040556f, 0.0025471006520092487f, 0.001994216348975897f, 0.0033309052232652903f, 0.0031217134092003107f, 0.0029116584919393063f, 0.00400872528553009f, 0.002408729400485754f, 0.0017202382441610098f, 0.0016316770343109965f, 0.003640643786638975f, 0.0030793959740549326f, 0.00352511671371758f, 0.0033396671060472727f, 0.004077686928212643f, 0.00220321211963892f, 0.004690246190875769f, 0.006782123353332281f, 0.0015401104465126991f, 0.0016030279221013188f, 0.0060477061197161674f, 0.0039008345920592546f, 0.004246728029102087f, 0.006472824607044458f, 0.002753028878942132f, 0.003983123693615198f, 0.0016098710475489497f, 0.0030075213871896267f, 0.0018369852332398295f, 0.0023582170251756907f, 0.0026167426258325577f, 0.003503885818645358f, 0.002074583200737834f, 0.003356166183948517f, 0.00326062785461545f, 0.0038140679243952036f, 0.0018566504586488008f, 0.0028541216161102057f, 0.0019197523361071944f, 0.003968079574406147f, 0.0016899328911677003f, 0.0015568918315693736f, 0.0031815876718610525f, 0.0019551466684788465f, 0.00153697794303298f, 0.004302786197513342f, 0.004382148385047913f, 0.002396190771833062f, 0.0033087613992393017f, 0.0029727770015597343f, 0.0016381146851927042f, 0.003288645064458251f, 0.0025634870398789644f, 0.002612548880279064f, 0.0013632414629682899f, 0.004316788166761398f, 0.002719235373660922f, 0.00414159195497632f, 0.019602447748184204f, 0.0027704734820872545f, 0.0049131279811263084f, 0.002047252608463168f, 0.0028863600455224514f, 0.0014724485808983445f, 0.0026759204920381308f, 0.002524270908907056f, 0.001544572995044291f, 0.0028152079321444035f, 0.0018247216939926147f, 0.0008005502168089151f, 0.0017318701138719916f, 0.002667846390977502f, 0.0024435375817120075f, 0.0033351804595440626f, 0.001983105903491378f, 0.002701948629692197f, 0.0030633974820375443f, 0.006887576077133417f, 0.0019330787472426891f, 0.0034255313221365213f, 0.003944917116314173f, 0.004623830318450928f, 0.0033964270260185003f, 0.0020740381442010403f, 0.002034478122368455f, 0.0023596056271344423f, 0.0025720137637108564f, 0.0030577583238482475f, 0.0033034789375960827f, 0.002495807595551014f, 0.0019808916840702295f, 0.003107560332864523f, 0.002560642547905445f, 0.0014840102521702647f, 0.002996726194396615f, 0.0016601652605459094f, 0.00346163148060441f, 0.002889502327889204f, 0.0018727799179032445f, 0.0033213242422789335f, 0.004195908550173044f, 0.003636915935203433f, 0.003566557541489601f, 0.002808714285492897f, 0.0021107213106006384f, 0.003199196420609951f, 0.002372247166931629f, 0.0024810777977108955f, 0.0022696848027408123f, 0.0028736598324030638f, 0.007251413073390722f, 0.004629835952073336f, 0.0013693689834326506f, 0.00272739096544683f, 0.002355224685743451f, 0.0026594800874590874f, 0.006485626567155123f, 0.004019103478640318f, 0.0011617622803896666f, 0.002254981081932783f, 0.0031111922580748796f, 0.0017208182252943516f, 0.003471230622380972f, 0.0026619145646691322f, 0.003624457400292158f, 0.002820212859660387f, 0.002217956120148301f, 0.0039434973150491714f, 0.003847819287329912f, 0.0031603146344423294f, 0.0028592925518751144f, 0.00440260348841548f, 0.00435918802395463f, 0.0017669220687821507f, 0.002664873842149973f, 0.0029614616651088f, 0.0031659200321882963f, 0.0052598402835428715f, 0.0029027217533439398f, 0.007071556057780981f, 0.0018580915639176965f, 0.00234760669991374f, 0.0025507332757115364f, 0.0025606334675103426f, 0.003452771110460162f, 0.0068435631692409515f, 0.0052425190806388855f, 0.003239340614527464f, 0.003588656196370721f, 0.0015470091020688415f, 0.0015429503982886672f, 0.002976057818159461f);
static const ai_layer_format_type conv2d_16_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_17_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_17_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_17_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_17_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_17_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_17_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_17_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_17_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_17_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_17_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_17_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_17_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_17_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.00701739639043808f, 0.004313140641897917f, 0.011653724126517773f, 0.002069976646453142f, 0.002021854743361473f, 0.00684460299089551f, 0.007899098098278046f, 0.0036973145324736834f, 0.004407911561429501f, 0.004239344969391823f, 0.003253036877140403f, 0.0024479394778609276f, 0.0030540835577994585f, 0.004697750322520733f, 0.0033520073629915714f, 0.0047746081836521626f, 0.006116810254752636f, 0.006203444208949804f, 0.009216371923685074f, 0.008278172463178635f, 0.007989299483597279f, 0.005512262228876352f, 0.004087362438440323f, 0.0063016377389431f, 0.0037858900614082813f, 0.011763214133679867f, 0.005069415085017681f, 0.0026963534764945507f, 0.005912918597459793f, 0.004431787878274918f, 0.006959998514503241f, 0.004282429348677397f, 0.003842886770144105f, 0.021301863715052605f, 0.0014251606771722436f, 0.0033619317691773176f, 0.009558496065437794f, 0.007855606265366077f, 0.007711054291576147f, 0.006063645239919424f, 0.0027842391282320023f, 0.008287638425827026f, 0.0027424441650509834f, 0.009363044053316116f, 0.004016592632979155f, 0.004959472920745611f, 0.002996872877702117f, 0.003046581521630287f, 0.004049882758408785f, 0.012115433812141418f, 0.0060470676980912685f, 0.003002719720825553f, 0.00907723605632782f, 0.0029257519636303186f, 0.008307043462991714f, 0.00303234183229506f, 0.005893469322472811f, 0.008729546330869198f, 0.009561270475387573f, 0.0031246505677700043f, 0.004114536102861166f, 0.006347055546939373f, 0.0031437156721949577f, 0.00401025265455246f, 0.005410635843873024f, 0.005466000642627478f, 0.005326094105839729f, 0.004915141500532627f, 0.0018459537532180548f, 0.004034931771457195f, 0.004590300843119621f, 0.005117167718708515f, 0.009318205527961254f, 0.004357585217803717f, 0.008591968566179276f, 0.005461694207042456f, 0.005770125426352024f, 0.007665539626032114f, 0.00841391459107399f, 0.004084183368831873f, 0.0015601752093061805f, 0.0035954236518591642f, 0.0037295417860150337f, 0.006865209899842739f, 0.004456018563359976f, 0.006482235621660948f, 0.007023265119642019f, 0.012060684151947498f, 0.006454986985772848f, 0.0033070389181375504f, 0.006940374616533518f, 0.010086153633892536f, 0.006499255541712046f, 0.0018898628186434507f, 0.012490080669522285f, 0.0038906829431653023f, 0.006655117496848106f, 0.004656394012272358f, 0.008548470214009285f, 0.005612500943243504f, 0.0026080762036144733f, 0.003929385915398598f, 0.005835425574332476f, 0.00830824114382267f, 0.005311740096658468f, 0.007998835295438766f, 0.007695791777223349f, 0.009547972120344639f, 0.003213301533833146f, 0.006508387625217438f, 0.0025826948694884777f, 0.006403859239071608f, 0.005230626557022333f, 0.010833005420863628f, 0.009330595843493938f, 0.007461609784513712f, 0.0021250476129353046f, 0.0020281504839658737f, 0.007132378872483969f, 0.00994948111474514f, 0.004900386556982994f, 0.0032001470681279898f, 0.01111047063022852f, 0.010665521025657654f, 0.009991354309022427f, 0.008120815269649029f, 0.003394057974219322f, 0.004690342582762241f, 0.0063791293650865555f, 0.006283428054302931f, 0.006369384005665779f, 0.006378167774528265f, 0.008593978360295296f, 0.004587558098137379f, 0.006213344167917967f, 0.0057676369324326515f, 0.008226346224546432f, 0.006075634621083736f, 0.006778324954211712f, 0.005811891984194517f, 0.01139598898589611f, 0.009326880797743797f, 0.006412893068045378f, 0.0025540560018271208f, 0.008556533604860306f, 0.012229005806148052f, 0.008202786557376385f, 0.009693554602563381f, 0.01401077676564455f, 0.00740552693605423f, 0.004615932237356901f, 0.004287160467356443f, 0.006418009754270315f, 0.007615365087985992f, 0.017632490023970604f, 0.0034864763729274273f, 0.005010826513171196f, 0.009857957251369953f, 0.01107539888471365f, 0.0043281009420752525f, 0.008167301304638386f, 0.0027106208726763725f, 0.004279480315744877f, 0.004179544281214476f, 0.003934166394174099f, 0.007389109116047621f, 0.0029920118395239115f, 0.009044992737472057f, 0.008268103003501892f, 0.004406766965985298f, 0.010265727527439594f, 0.00757457735016942f, 0.007869224064052105f, 0.013575226068496704f, 0.008210277184844017f, 0.007956551387906075f, 0.005302175879478455f, 0.006461828015744686f, 0.002788519486784935f, 0.008187644183635712f, 0.006211466621607542f, 0.004346243105828762f, 0.004799659829586744f, 0.004116152413189411f, 0.0016021586488932371f, 0.0035429971758276224f, 0.008311227895319462f, 0.007822318933904171f, 0.01309432927519083f, 0.00548219308257103f, 0.004537029191851616f, 0.004309321753680706f, 0.00463853171095252f, 0.005305186379700899f, 0.006710279732942581f, 0.006895487662404776f, 0.007403925061225891f, 0.01106946263462305f, 0.008566748350858688f, 0.007203591987490654f, 0.003761963453143835f, 0.007442000787705183f, 0.007881838828325272f, 0.0037314442452043295f, 0.0062866052612662315f, 0.0028121008072048426f, 0.003581209108233452f, 0.0023479412775486708f, 0.00757640041410923f, 0.004723731894046068f, 0.008255839347839355f, 0.005373320542275906f, 0.008708733133971691f, 0.011612669564783573f, 0.006198157090693712f, 0.004690016154199839f, 0.005721531342715025f, 0.007666668854653835f, 0.005298667121678591f, 0.0040869442746043205f, 0.004146952647715807f, 0.002608745126053691f, 0.014154360629618168f, 0.009374129585921764f, 0.0056351046077907085f, 0.009125003591179848f, 0.0073750680312514305f, 0.003560767974704504f, 0.005822517443448305f, 0.0018007083563134074f, 0.009199605323374271f, 0.0032798482570797205f, 0.00360857043415308f, 0.01018989086151123f, 0.0048957145772874355f, 0.006572893355041742f, 0.005125630181282759f, 0.010756448842585087f, 0.007513842079788446f, 0.003248203080147505f, 0.003660457208752632f, 0.0010343866888433695f, 0.011517232283949852f, 0.0021134624257683754f, 0.007491969969123602f, 0.005898182280361652f, 0.002932347822934389f, 0.007195374928414822f, 0.002550015924498439f, 0.003987496718764305f, 0.003964819945394993f, 0.006066930014640093f, 0.0016965498216450214f, 0.012157827615737915f, 0.0060923416167497635f, 0.0076858229003846645f);
static const ai_u16 conv2d_17_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_17_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_18_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_18_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_18_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_18_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_18_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_18_t_out_0_shape_ch_const_u16 = 256;
static const ai_i8 conv2d_18_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_18_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_18_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_18_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_18_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002307844115421176f, 0.00219013006426394f, 0.0026629138737916946f, 0.0026625595055520535f, 0.0028876191936433315f, 0.004317606333643198f, 0.004837597254663706f, 0.0011949983891099691f, 0.003364293836057186f, 0.00209055095911026f, 0.004202407319098711f, 0.003713326994329691f, 0.001971205696463585f, 0.0030812665354460478f, 0.002947622211650014f, 0.003310955362394452f, 0.002721372526139021f, 0.0030903678853064775f, 0.003044064622372389f, 0.00547356391325593f, 0.005680640693753958f, 0.002641822211444378f, 0.0037985320668667555f, 0.0009575636358931661f, 0.0030310957226902246f, 0.002564528491348028f, 0.002397476462647319f, 0.004340960178524256f, 0.0029451504815369844f, 0.0021990810055285692f, 0.003412532852962613f, 0.0028433753177523613f, 0.0020996304228901863f, 0.0032522091642022133f, 0.002757004462182522f, 0.0024442714639008045f, 0.0015798911917954683f, 0.00487756822258234f, 0.0027906170580536127f, 0.002591329161077738f, 0.0017504253191873431f, 0.0054536196403205395f, 0.0026604686863720417f, 0.0037025504279881716f, 0.005790109746158123f, 0.0027181925252079964f, 0.0017658458091318607f, 0.002607414498925209f, 0.0012508172076195478f, 0.002001354470849037f, 0.005939531605690718f, 0.0048300051130354404f, 0.002321125939488411f, 0.0061171636916697025f, 0.0027963502798229456f, 0.002134463284164667f, 0.0034883981570601463f, 0.003238329663872719f, 0.00209631840698421f, 0.0010353599209338427f, 0.0020004198886454105f, 0.0020324881188571453f, 0.0038695449475198984f, 0.008658775128424168f, 0.006030620075762272f, 0.005167235620319843f, 0.0030566200148314238f, 0.004873733967542648f, 0.00298451934941113f, 0.0028143050149083138f, 0.001939881476573646f, 0.0028648769948631525f, 0.0028941223863512278f, 0.005523908417671919f, 0.0020638671703636646f, 0.0022621992975473404f, 0.004488167352974415f, 0.0024209395051002502f, 0.0026075609494000673f, 0.006126324646174908f, 0.002465334488078952f, 0.0014387271367013454f, 0.0028791434597223997f, 0.004211099818348885f, 0.003582804696634412f, 0.0033902365248650312f, 0.003840277437120676f, 0.0015562925254926085f, 0.004550810903310776f, 0.006415234412997961f, 0.008756401017308235f, 0.002091149566695094f, 0.0016280004056170583f, 0.0018020825227722526f, 0.0026181438006460667f, 0.0017581179272383451f, 0.002767066704109311f, 0.0026989318430423737f, 0.002373943803831935f, 0.0023971626069396734f, 0.003997283987700939f, 0.0028287768363952637f, 0.0042023672722280025f, 0.0022559466306120157f, 0.0033382487017661333f, 0.001658121938817203f, 0.0026869040448218584f, 0.002174832159653306f, 0.003334094537422061f, 0.0028674760833382607f, 0.0019105268875136971f, 0.003404373535886407f, 0.0022927604150027037f, 0.0019210425671190023f, 0.0015521679306402802f, 0.0020322143100202084f, 0.002380586927756667f, 0.007541971281170845f, 0.0021417380776256323f, 0.004644096828997135f, 0.003946206066757441f, 0.0029970393516123295f, 0.0029365450609475374f, 0.0014543873257935047f, 0.004240753129124641f, 0.0039253816939890385f, 0.001678071217611432f, 0.005732090212404728f, 0.0019083794904872775f, 0.0016940388595685363f, 0.0037868276704102755f, 0.0018305485136806965f, 0.0030171654652804136f, 0.002026728354394436f, 0.007478079758584499f, 0.001184950233437121f, 0.003813648596405983f, 0.0041804187931120396f, 0.00343603128567338f, 0.0032664949540048838f, 0.003917269874364138f, 0.00742725795134902f, 0.002664426574483514f, 0.002363633131608367f, 0.005240616854280233f, 0.0012652279110625386f, 0.0031457075383514166f, 0.002296990714967251f, 0.0033765023108571768f, 0.00395440636202693f, 0.0020472947508096695f, 0.00324470573104918f, 0.0035648576449602842f, 0.0025870443787425756f, 0.002887999638915062f, 0.004101024474948645f, 0.00216006045229733f, 0.0019350498914718628f, 0.0032507088035345078f, 0.0031403186731040478f, 0.0028377603739500046f, 0.0034960457123816013f, 0.0036033045034855604f, 0.002517975866794586f, 0.0023491443134844303f, 0.0035148293245583773f, 0.001933123916387558f, 0.003774578683078289f, 0.0018344537820667028f, 0.0035649382043629885f, 0.002999345539137721f, 0.0020909328013658524f, 0.0013311674119904637f, 0.0033310852013528347f, 0.0007433809223584831f, 0.0012862575240433216f, 0.0038036233745515347f, 0.0036351208109408617f, 0.0029873563908040524f, 0.0032267686910927296f, 0.0035269660875201225f, 0.004585317336022854f, 0.002287535695359111f, 0.005712264683097601f, 0.004008902236819267f, 0.002152542117983103f, 0.0034688247833400965f, 0.00505368085578084f, 0.002836422761902213f, 0.0023021709639579058f, 0.0019467838574200869f, 0.0039046823512762785f, 0.00247932062484324f, 0.002021003747358918f, 0.004709797445684671f, 0.0017832532757893205f, 0.003159038722515106f, 0.003465272020548582f, 0.0034707714803516865f, 0.002624070504680276f, 0.0019249578472226858f, 0.003770912066102028f, 0.002672361209988594f, 0.004264320712536573f, 0.008853544481098652f, 0.0026135824155062437f, 0.004424779210239649f, 0.002138097770512104f, 0.0027979351580142975f, 0.001582142896950245f, 0.0026437044143676758f, 0.002983100013807416f, 0.0023349858820438385f, 0.006386246997863054f, 0.002079644240438938f, 0.003837070195004344f, 0.0037911480758339167f, 0.003896618727594614f, 0.0027575092390179634f, 0.003220716491341591f, 0.00417555496096611f, 0.0020449769217520952f, 0.0038698348216712475f, 0.0019327773479744792f, 0.004374442622065544f, 0.0019360219594091177f, 0.00122356996871531f, 0.004688906483352184f, 0.0031640720553696156f, 0.003349280683323741f, 0.005050353240221739f, 0.0024223760701715946f, 0.0030758243519812822f, 0.0022060845512896776f, 0.002295035868883133f, 0.0033563319593667984f, 0.0031303430441766977f, 0.003788167843595147f, 0.003124574199318886f, 0.004012691788375378f, 0.0021034162491559982f, 0.004212541505694389f, 0.003177000442519784f, 0.002285559196025133f, 0.002399468794465065f, 0.0017280519241467118f, 0.002431621542200446f, 0.002242794493213296f, 0.0052458480931818485f, 0.002446977887302637f, 0.00135618238709867f, 0.002980664139613509f, 0.003092451486736536f, 0.0023697337601333857f, 0.0037031800020486116f, 0.0022658805828541517f);
static const ai_layer_format_type conv2d_18_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_19_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_19_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_19_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_19_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_19_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_19_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_19_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_19_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_19_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_19_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_19_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_19_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_19_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0036780398804694414f, 0.00503570307046175f, 0.01825217716395855f, 0.0039009933825582266f, 0.004456113092601299f, 0.004041628446429968f, 0.0010115074692294002f, 0.0075023844838142395f, 0.0022065266966819763f, 0.01093218382447958f, 0.0016356653068214655f, 0.0016261483542621136f, 0.007481779903173447f, 0.0033298805356025696f, 0.005376156885176897f, 0.007012035697698593f, 0.004864076618105173f, 0.006326759699732065f, 0.0030320510268211365f, 0.00130456464830786f, 0.0019672925118356943f, 0.008585544303059578f, 0.002667167689651251f, 0.021256988868117332f, 0.0036509251222014427f, 0.01362970843911171f, 0.007665921002626419f, 0.002373035764321685f, 0.006442959886044264f, 0.0034310459159314632f, 0.004148208536207676f, 0.005726775620132685f, 0.009619002230465412f, 0.006988798268139362f, 0.0053924196399748325f, 0.005023042671382427f, 0.01000479981303215f, 0.005946268793195486f, 0.00834107119590044f, 0.0028077031020075083f, 0.008160333149135113f, 0.005826398730278015f, 0.002715824404731393f, 0.013757611624896526f, 0.00581662543118f, 0.011985745280981064f, 0.005103581584990025f, 0.004531561397016048f, 0.012531961314380169f, 0.0063833086751401424f, 0.0011498165549710393f, 0.0029802743811160326f, 0.007161428220570087f, 0.007493537850677967f, 0.0067181880585849285f, 0.010212358087301254f, 0.004656339064240456f, 0.011936220340430737f, 0.004172197077423334f, 0.014276642352342606f, 0.00834321603178978f, 0.009545372799038887f, 0.0015688214916735888f, 0.0021719266660511494f, 0.0031836240086704493f, 0.0041400664485991f, 0.004765221383422613f, 0.004610361531376839f, 0.002280212240293622f, 0.008734239265322685f, 0.011444458737969398f, 0.002953136572614312f, 0.007060843054205179f, 0.004738063085824251f, 0.006425384432077408f, 0.008822236210107803f, 0.003070719074457884f, 0.006812897510826588f, 0.00430652080103755f, 0.0038526952266693115f, 0.003958237823098898f, 0.003912112209945917f, 0.009425511583685875f, 0.013631610199809074f, 0.010657642967998981f, 0.0033024121075868607f, 0.006367286667227745f, 0.008377864956855774f, 0.004144475795328617f, 0.003464974695816636f, 0.00856718048453331f, 0.008772078901529312f, 0.009551656432449818f, 0.0029642190784215927f, 0.004905310925096273f, 0.007224326953291893f, 0.006353071890771389f, 0.005941792856901884f, 0.0030148623045533895f, 0.00784795731306076f, 0.001508113811723888f, 0.0055093951523303986f, 0.0034868912771344185f, 0.00263746059499681f, 0.00791599228978157f, 0.01003195345401764f, 0.0042582545429468155f, 0.0058981310576200485f, 0.005038220435380936f, 0.00759965879842639f, 0.008085516281425953f, 0.006828914862126112f, 0.004121892154216766f, 0.007420026697218418f, 0.011317478492856026f, 0.007648838218301535f, 0.0029871000442653894f, 0.0017164673190563917f, 0.00938190147280693f, 0.005152295343577862f, 0.003353955689817667f, 0.003804826643317938f, 0.004387053195387125f, 0.01570424623787403f, 0.006801979150623083f, 0.006911470089107752f, 0.01076118741184473f, 0.0012095766142010689f, 0.008933176286518574f, 0.002271595411002636f, 0.007275660056620836f, 0.006479315459728241f, 0.008283707313239574f, 0.006022345740348101f, 0.001107699004933238f, 0.014807651750743389f, 0.001263582264073193f, 0.005983705632388592f, 0.00325163290835917f, 0.004619373939931393f, 0.007646877318620682f, 0.0013365806080400944f, 0.0015415882226079702f, 0.007564497645944357f, 0.0016280628042295575f, 0.01432974822819233f, 0.005346114281564951f, 0.006040305830538273f, 0.005749888718128204f, 0.004170488100498915f, 0.009142749011516571f, 0.003505312604829669f, 0.003948126453906298f, 0.0030669462867081165f, 0.0040509505197405815f, 0.0023885550908744335f, 0.00786739494651556f, 0.013022257015109062f, 0.0029065620619803667f, 0.006333719473332167f, 0.004508680664002895f, 0.005160670727491379f, 0.0032848024275153875f, 0.008324431255459785f, 0.006266677752137184f, 0.004752004519104958f, 0.006693634670227766f, 0.009031259454786777f, 0.005908006336539984f, 0.002382191363722086f, 0.008864359930157661f, 0.005093705374747515f, 0.009874142706394196f, 0.00757344625890255f, 0.014784470200538635f, 0.006897095125168562f, 0.0028655703645199537f, 0.00342393945902586f, 0.006669455673545599f, 0.006506039761006832f, 0.004638871178030968f, 0.0009752784972079098f, 0.010660136118531227f, 0.0014003992546349764f, 0.0067573534324765205f, 0.0025101571809500456f, 0.008236533030867577f, 0.0015055410331115127f, 0.007352964952588081f, 0.009041870012879372f, 0.007807603571563959f, 0.0016541871009394526f, 0.003770087845623493f, 0.004643116146326065f, 0.0041625015437603f, 0.015952277928590775f, 0.005001853220164776f, 0.007642807438969612f, 0.007955185137689114f, 0.00867284182459116f, 0.004414638038724661f, 0.004626296926289797f, 0.005727310664951801f, 0.004065352492034435f, 0.006768502295017242f, 0.010560971684753895f, 0.005267679691314697f, 0.005498135462403297f, 0.001929928665049374f, 0.00721755949780345f, 0.0036224157083779573f, 0.007715886924415827f, 0.003447874914854765f, 0.006972297560423613f, 0.005932389292865992f, 0.0040845712646842f, 0.0038873017765581608f, 0.0012541419127956033f, 0.004175652749836445f, 0.0037737111561000347f, 0.0009895829716697335f, 0.005873163230717182f, 0.004603601060807705f, 0.003461984684690833f, 0.0022006710059940815f, 0.010310107842087746f, 0.011152327992022038f, 0.0033248644322156906f, 0.0020865316037088633f, 0.0009992695413529873f, 0.0023548651952296495f, 0.007854871451854706f, 0.008705035783350468f, 0.007203401532024145f, 0.004298771265894175f, 0.0024588333908468485f, 0.010272044688463211f, 0.005226811394095421f, 0.00646794680505991f, 0.009848720394074917f, 0.01116700004786253f, 0.0033395031932741404f, 0.0015524054178968072f, 0.0038542835973203182f, 0.00857734028249979f, 0.008125057443976402f, 0.00838448479771614f, 0.012395418249070644f, 0.0032675028778612614f, 0.00850769504904747f, 0.007276054006069899f, 0.004312011878937483f, 0.004799856338649988f, 0.005330451298505068f, 0.005371068138629198f, 0.008418221026659012f);
static const ai_u16 conv2d_19_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_19_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_20_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_20_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_20_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_20_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_20_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_20_t_out_0_shape_ch_const_u16 = 256;
static const ai_i8 conv2d_20_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_20_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_20_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_20_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_20_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0027839839458465576f, 0.003373331157490611f, 0.003200383158400655f, 0.0031669875606894493f, 0.00323952897451818f, 0.007939504459500313f, 0.0022535950411111116f, 0.002293126890435815f, 0.003895148169249296f, 0.00526104960590601f, 0.006471958011388779f, 0.002212155144661665f, 0.003176657948642969f, 0.002189731691032648f, 0.003577055409550667f, 0.002382138976827264f, 0.0022967772092670202f, 0.006244657561182976f, 0.003578358795493841f, 0.004037870094180107f, 0.004697919823229313f, 0.0029981897678226233f, 0.002784116193652153f, 0.0036006025038659573f, 0.005038927774876356f, 0.003079305635765195f, 0.0028514054138213396f, 0.0033008577302098274f, 0.002046645386144519f, 0.0019962098449468613f, 0.0042520673014223576f, 0.004870473872870207f, 0.004828345961868763f, 0.0034054652787745f, 0.0023250305093824863f, 0.0021401240956038237f, 0.0033597552683204412f, 0.0016460155602544546f, 0.0028738724067807198f, 0.0012375558726489544f, 0.0037331879138946533f, 0.002157587790861726f, 0.0028174472972750664f, 0.0035266242921352386f, 0.0027121519669890404f, 0.001902969554066658f, 0.0031201031524688005f, 0.004076592158526182f, 0.003443316323682666f, 0.0033153733238577843f, 0.003431913908571005f, 0.003677181201055646f, 0.002349303802475333f, 0.003465046640485525f, 0.00334739126265049f, 0.0028965144883841276f, 0.0025894837453961372f, 0.002909060101956129f, 0.002867171773687005f, 0.00368083780631423f, 0.004693671129643917f, 0.006571643985807896f, 0.003233024151995778f, 0.005503723863512278f, 0.00328633189201355f, 0.002388023305684328f, 0.002333584940060973f, 0.0017019796650856733f, 0.004577158484607935f, 0.004351277835667133f, 0.0034753563813865185f, 0.002819898072630167f, 0.002976964460685849f, 0.0032186720054596663f, 0.003064825665205717f, 0.0025339743588119745f, 0.0060922978445887566f, 0.0017164625460281968f, 0.0021215835586190224f, 0.002908741356804967f, 0.0013710384955629706f, 0.003734803758561611f, 0.01017043087631464f, 0.003201969899237156f, 0.003728428389877081f, 0.003182120155543089f, 0.0037439893931150436f, 0.002706637606024742f, 0.00350981205701828f, 0.003718743333593011f, 0.0036644944921135902f, 0.002707548439502716f, 0.004721496719866991f, 0.00251740007661283f, 0.0036957855336368084f, 0.0028340513817965984f, 0.003958866465836763f, 0.0020652678795158863f, 0.0023984203580766916f, 0.004462856333702803f, 0.0019165629055351019f, 0.004961255006492138f, 0.0023100811522454023f, 0.00232426798902452f, 0.003622584044933319f, 0.004504140000790358f, 0.002449069172143936f, 0.006613635923713446f, 0.0023169631604105234f, 0.0055501461029052734f, 0.0018131373217329383f, 0.0027077491395175457f, 0.004442609380930662f, 0.002876835875213146f, 0.0020722304470837116f, 0.004902324639260769f, 0.0031165117397904396f, 0.0040412782691419125f, 0.0024255593307316303f, 0.003964252304285765f, 0.0026419872883707285f, 0.0032880627550184727f, 0.0031483029015362263f, 0.0030796423088759184f, 0.0029166550375521183f, 0.0035683722235262394f, 0.009010323323309422f, 0.0032935081981122494f, 0.003156352788209915f, 0.00271600135602057f, 0.0037324856966733932f, 0.0027892407961189747f, 0.002683281200006604f, 0.002602722030133009f, 0.002727207727730274f, 0.0037591455038636923f, 0.0027173031121492386f, 0.003569612745195627f, 0.0020365160889923573f, 0.003330591833218932f, 0.0020444453693926334f, 0.0023338880855590105f, 0.004428383894264698f, 0.003627939848229289f, 0.004905890207737684f, 0.005551858339458704f, 0.0018270822474732995f, 0.0035657549742609262f, 0.006036716978996992f, 0.0032768435776233673f, 0.003414832754060626f, 0.0025523477233946323f, 0.002770769875496626f, 0.0033794622868299484f, 0.00444150622934103f, 0.001801424310542643f, 0.00228521809913218f, 0.002197618829086423f, 0.002773101907223463f, 0.003981389105319977f, 0.0062997774221003056f, 0.002928843954578042f, 0.0026058994699269533f, 0.0013972193701192737f, 0.003891504369676113f, 0.0027924024034291506f, 0.0024356506764888763f, 0.005919794086366892f, 0.0029836094472557306f, 0.0013338271528482437f, 0.002522129099816084f, 0.004827924072742462f, 0.002645554021000862f, 0.004432026296854019f, 0.0026439798530191183f, 0.0037458352744579315f, 0.0054024565033614635f, 0.003566898638382554f, 0.0023250540252774954f, 0.003818454220890999f, 0.001034819521009922f, 0.003301797667518258f, 0.004571366589516401f, 0.0028064383659511805f, 0.0022018130403012037f, 0.0028597251512110233f, 0.0030475384555757046f, 0.0029923380352556705f, 0.0035222896840423346f, 0.0037601995281875134f, 0.0058991205878555775f, 0.004629069939255714f, 0.002208129270002246f, 0.002490284852683544f, 0.0031187967397272587f, 0.0032571733463555574f, 0.0031705519650131464f, 0.004708740394562483f, 0.0030153156258165836f, 0.002916131867095828f, 0.009152467362582684f, 0.0034791119396686554f, 0.002659582532942295f, 0.0023266267962753773f, 0.005786878522485495f, 0.002773609012365341f, 0.00506468303501606f, 0.002594792051240802f, 0.0028676914516836405f, 0.0032275416888296604f, 0.006058960221707821f, 0.003959368914365768f, 0.002033547731116414f, 0.003684814553707838f, 0.0022630144376307726f, 0.004213624633848667f, 0.005742806941270828f, 0.0034405523911118507f, 0.0016493264120072126f, 0.0030285806860774755f, 0.003760513151064515f, 0.0043001361191272736f, 0.0033575270790606737f, 0.006251850165426731f, 0.002537516411393881f, 0.0026660203002393246f, 0.0038076338823884726f, 0.004593963734805584f, 0.003555468050763011f, 0.00523412087932229f, 0.0028655410278588533f, 0.0023243052419275045f, 0.0018962635658681393f, 0.0030123696196824312f, 0.004692799877375364f, 0.00603873236104846f, 0.00240082829259336f, 0.0022097579203546047f, 0.0030610752291977406f, 0.0036387103609740734f, 0.003349758917465806f, 0.002333912067115307f, 0.0029131779447197914f, 0.0028833518736064434f, 0.003602153854444623f, 0.0032874157186597586f, 0.0028765792958438396f, 0.0032195881940424442f, 0.0034230039454996586f, 0.003333183703944087f, 0.0022410484962165356f, 0.002341404091566801f, 0.0031015435233712196f, 0.003661196446046233f, 0.003452005097642541f, 0.0038329416420310736f);
static const ai_layer_format_type conv2d_20_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_21_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_21_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_21_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_21_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_21_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_21_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_21_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_21_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_21_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_21_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_21_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_21_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_21_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.008608676493167877f, 0.005093743558973074f, 0.00842306762933731f, 0.004143246449530125f, 0.010933873243629932f, 0.0025084656663239002f, 0.0041817110031843185f, 0.0031267134472727776f, 0.0044391220435500145f, 0.0045168702490627766f, 0.0020429440774023533f, 0.007098891772329807f, 0.003217305988073349f, 0.00993227306753397f, 0.0028473674319684505f, 0.005387549288570881f, 0.009726335294544697f, 0.002601924818009138f, 0.006951379124075174f, 0.0023713838309049606f, 0.006952862720936537f, 0.004293152596801519f, 0.0057656108401715755f, 0.008179609663784504f, 0.005442996043711901f, 0.008887670002877712f, 0.0030471296049654484f, 0.0025266229640692472f, 0.009010949172079563f, 0.00449322210624814f, 0.005734394304454327f, 0.0015456717228516936f, 0.0017213047249242663f, 0.002586834831163287f, 0.007037872448563576f, 0.006379891652613878f, 0.004150379914790392f, 0.004777558613568544f, 0.004638089332729578f, 0.013766005635261536f, 0.00404747761785984f, 0.0034290680196136236f, 0.004018101841211319f, 0.00448883231729269f, 0.0033857435919344425f, 0.011826805770397186f, 0.0055175828747451305f, 0.005745218135416508f, 0.003913138993084431f, 0.005873527843505144f, 0.005990180186927319f, 0.005505533888936043f, 0.01250979769974947f, 0.0029709404334425926f, 0.004407425876706839f, 0.0026280353777110577f, 0.008801279589533806f, 0.005978087428957224f, 0.0054705385118722916f, 0.006822173483669758f, 0.006871765945106745f, 0.00335329701192677f, 0.007540146820247173f, 0.0014809188432991505f, 0.009427453391253948f, 0.007721848785877228f, 0.002825496718287468f, 0.0074137235060334206f, 0.009990730322897434f, 0.009001472033560276f, 0.002625161549076438f, 0.0017181382281705737f, 0.002383209066465497f, 0.005564468912780285f, 0.0032803043723106384f, 0.005067398305982351f, 0.007935401052236557f, 0.01241414062678814f, 0.008225781843066216f, 0.0015419030096381903f, 0.004670575261116028f, 0.004275635816156864f, 0.0025532268919050694f, 0.004823778755962849f, 0.001769497524946928f, 0.006808076519519091f, 0.0035286874044686556f, 0.0029012919403612614f, 0.0028723962604999542f, 0.007222516927868128f, 0.0015056204283609986f, 0.008370822295546532f, 0.001791749382391572f, 0.0030632552225142717f, 0.0055199735797941685f, 0.00530611164867878f, 0.0034651937894523144f, 0.0075761619955301285f, 0.009300289675593376f, 0.009099332615733147f, 0.007085640914738178f, 0.00440228171646595f, 0.006235022097826004f, 0.006390395574271679f, 0.0021876220125705004f, 0.004811335355043411f, 0.0077696009539067745f, 0.0023291767574846745f, 0.006321072578430176f, 0.0014941298868507147f, 0.00804012082517147f, 0.00735687930136919f, 0.0023829499259591103f, 0.0024917335249483585f, 0.003113222774118185f, 0.003139146836474538f, 0.0012492815731093287f, 0.0024548075161874294f, 0.00439464021474123f, 0.009105289354920387f, 0.00699731195345521f, 0.0047989701852202415f, 0.005663065239787102f, 0.009127417579293251f, 0.002167564583942294f, 0.005477497819811106f, 0.0016812104731798172f, 0.0034624345134943724f, 0.0075613753870129585f, 0.006777739152312279f, 0.005152634344995022f, 0.004461157601326704f, 0.005235431250184774f, 0.004019021987915039f, 0.0019593914039433002f, 0.0026957469526678324f, 0.003913357388228178f, 0.008158626966178417f, 0.014308663085103035f, 0.0018394768703728914f, 0.005521038081496954f, 0.005411418154835701f, 0.0022962833754718304f, 0.0034282070118933916f, 0.005144266411662102f, 0.0037442869506776333f, 0.00440521165728569f, 0.004137046635150909f, 0.00805502850562334f, 0.0012250435538589954f, 0.002492193365469575f, 0.00684516504406929f, 0.00768390903249383f, 0.0013998381327837706f, 0.00524838175624609f, 0.006858612410724163f, 0.0029932253528386354f, 0.005443197675049305f, 0.005660972557961941f, 0.005102067720144987f, 0.0035683654714375734f, 0.003470764961093664f, 0.002977077616378665f, 0.008168136700987816f, 0.004582975059747696f, 0.007638264913111925f, 0.004154338035732508f, 0.006436534691601992f, 0.006299666129052639f, 0.01333586871623993f, 0.006514133885502815f, 0.005118423607200384f, 0.002612057141959667f, 0.007684392388910055f, 0.005345131270587444f, 0.0017295771976932883f, 0.0069796438328921795f, 0.005380562506616116f, 0.007212257012724876f, 0.003922483418136835f, 0.011962201446294785f, 0.006153787951916456f, 0.005251386668533087f, 0.004368449095636606f, 0.01198528427630663f, 0.008642510510981083f, 0.003393554361537099f, 0.003006687154993415f, 0.002625588560476899f, 0.005971143953502178f, 0.0013452749699354172f, 0.001751381903886795f, 0.015119981952011585f, 0.011385908350348473f, 0.006677975878119469f, 0.002634367672726512f, 0.0017872791504487395f, 0.003114377148449421f, 0.0026457582134753466f, 0.0061113727279007435f, 0.002219537040218711f, 0.005105406977236271f, 0.020151596516370773f, 0.004285513423383236f, 0.004539456218481064f, 0.0030049553606659174f, 0.003907329402863979f, 0.003348366357386112f, 0.002754399087280035f, 0.004942859522998333f, 0.003148090559989214f, 0.010960649698972702f, 0.006079036742448807f, 0.0014757565222680569f, 0.005816827528178692f, 0.005660150665789843f, 0.0023109125904738903f, 0.0014228804502636194f, 0.008128347806632519f, 0.005806231405586004f, 0.005428648553788662f, 0.0046866112388670444f, 0.0018412154167890549f, 0.0018401590641587973f, 0.014042699709534645f, 0.002833024365827441f, 0.001460044994018972f, 0.0060903276316821575f, 0.001153679913841188f, 0.002423160243779421f, 0.0021233055740594864f, 0.002937393030151725f, 0.004342024214565754f, 0.0022958707995712757f, 0.007417996879667044f, 0.0015377044910565019f, 0.006760930642485619f, 0.005939907394349575f, 0.006920045707374811f, 0.0034759226255118847f, 0.0034136713948100805f, 0.006718730088323355f, 0.003911578096449375f, 0.004658908117562532f, 0.006440708879381418f, 0.005903771612793207f, 0.0033147253561764956f, 0.005103080999106169f, 0.004350799135863781f, 0.0015496850246563554f, 0.008739249780774117f, 0.003830208210274577f, 0.0038574212230741978f, 0.004970179405063391f, 0.002686395077034831f, 0.002475870307534933f);
static const ai_u16 conv2d_21_t_out_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_21_t_out_0_shape_h_const_u16 = 14;

static const ai_u16 conv2d_22_t_in_0_shape_w_const_u16 = 14;
static const ai_u16 conv2d_22_t_in_0_shape_h_const_u16 = 14;
static const ai_u16 conv2d_22_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_22_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_22_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_22_t_out_0_shape_ch_const_u16 = 256;
static const ai_i8 conv2d_22_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_22_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_22_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_22_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_22_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0032872154843062162f, 0.004620458465069532f, 0.001620217808522284f, 0.0025542646180838346f, 0.0028249681927263737f, 0.0030080792494118214f, 0.002877402352169156f, 0.001429438591003418f, 0.001777041470631957f, 0.0031538468319922686f, 0.00315962266176939f, 0.00115920917596668f, 0.0025345799513161182f, 0.0032840827479958534f, 0.002399734454229474f, 0.0059257145039737225f, 0.009240309707820415f, 0.0035588762257248163f, 0.0018719141371548176f, 0.002214364940300584f, 0.0024434360675513744f, 0.0025075331795960665f, 0.002727842191234231f, 0.005091267172247171f, 0.004209297709167004f, 0.003708201227709651f, 0.0033882635179907084f, 0.002923038322478533f, 0.003540497040376067f, 0.002302184235304594f, 0.0024742698296904564f, 0.0031070003751665354f, 0.0034980610944330692f, 0.00681638065725565f, 0.0015106333885341883f, 0.0014409334398806095f, 0.003199836937710643f, 0.002814972074702382f, 0.003482378553599119f, 0.004061144310981035f, 0.0031442861072719097f, 0.0036920441780239344f, 0.001205137581564486f, 0.003492343472316861f, 0.0018650226993486285f, 0.0020715210121124983f, 0.004135792143642902f, 0.0032402263022959232f, 0.003062748583033681f, 0.0042408243753015995f, 0.0013399310410022736f, 0.002161092357710004f, 0.003170102834701538f, 0.002916982863098383f, 0.002790826605632901f, 0.0020784875378012657f, 0.008939158171415329f, 0.0025940039195120335f, 0.0030287920963019133f, 0.0029414710588753223f, 0.004870739299803972f, 0.0034883080516010523f, 0.002086926018819213f, 0.004834525752812624f, 0.004656949546188116f, 0.003275415161624551f, 0.0016758869169279933f, 0.00286278803832829f, 0.0023846460971981287f, 0.002303161658346653f, 0.003095547901466489f, 0.004132923204451799f, 0.0038048650603741407f, 0.0034043912310153246f, 0.002662844490259886f, 0.003501742146909237f, 0.0036353853065520525f, 0.006426546722650528f, 0.002520335838198662f, 0.0030387539882212877f, 0.0038329095114022493f, 0.0023140073753893375f, 0.0025444752536714077f, 0.004730558954179287f, 0.0017192927189171314f, 0.0035434761084616184f, 0.002808118239045143f, 0.003671183018013835f, 0.0030581278260797262f, 0.002978614764288068f, 0.002479277551174164f, 0.004028079099953175f, 0.001972189173102379f, 0.0032235635444521904f, 0.0034568984992802143f, 0.0019393112743273377f, 0.0013431047555059195f, 0.0017482966650277376f, 0.0038174381479620934f, 0.003333563916385174f, 0.003155970945954323f, 0.0016005774959921837f, 0.0015443109441548586f, 0.0034013211261481047f, 0.002506854012608528f, 0.0024673815350979567f, 0.0037636267952620983f, 0.0026250951923429966f, 0.0016680407570675015f, 0.0028854929842054844f, 0.0030380520038306713f, 0.002855564933270216f, 0.0013825735077261925f, 0.0030481056310236454f, 0.0033984151668846607f, 0.0021336774807423353f, 0.005167946219444275f, 0.002636796562001109f, 0.0032818808685988188f, 0.0034752851352095604f, 0.0030041339341551065f, 0.0022210129536688328f, 0.002821259433403611f, 0.0015838807448744774f, 0.0013285006862133741f, 0.0028478186577558517f, 0.0035670027136802673f, 0.0021937573328614235f, 0.002597054000943899f, 0.003780744504183531f, 0.003283197758719325f, 0.004122428596019745f, 0.0012561660259962082f, 0.003009803593158722f, 0.0019528298871591687f, 0.003553644521161914f, 0.00353225483559072f, 0.0027022678405046463f, 0.0027367188595235348f, 0.0033041047863662243f, 0.004929294344037771f, 0.0022919820621609688f, 0.0037901289761066437f, 0.003604855155572295f, 0.002968136454001069f, 0.0030855878721922636f, 0.002270669210702181f, 0.0038093857001513243f, 0.0016383588081225753f, 0.0029080919921398163f, 0.0026705849450081587f, 0.005347928497940302f, 0.0016361318994313478f, 0.0025748382322490215f, 0.005218722391873598f, 0.0030693490989506245f, 0.0033505517058074474f, 0.0029715909622609615f, 0.002971882466226816f, 0.0016083456575870514f, 0.0019069378031417727f, 0.002909173257648945f, 0.0018601245246827602f, 0.0020636599510908127f, 0.001155623234808445f, 0.00449416832998395f, 0.004953958094120026f, 0.0023475305642932653f, 0.0029292914550751448f, 0.0034589588176459074f, 0.0021880674175918102f, 0.0014228133950382471f, 0.002706247614696622f, 0.0026848192792385817f, 0.001443639281205833f, 0.0042266943491995335f, 0.0028248014859855175f, 0.0028939777985215187f, 0.0022687348537147045f, 0.005045533645898104f, 0.0035061994567513466f, 0.0024009710177779198f, 0.0033726331312209368f, 0.004329099785536528f, 0.0030304165557026863f, 0.002571818418800831f, 0.003950618673115969f, 0.0035349521785974503f, 0.0035135853104293346f, 0.00223178556188941f, 0.0018926224438473582f, 0.0033553186804056168f, 0.003238827921450138f, 0.00380150368437171f, 0.0017464549746364355f, 0.002755683148279786f, 0.0035196018870919943f, 0.002259257948026061f, 0.0021206634119153023f, 0.0017717195441946387f, 0.0025697569362819195f, 0.003845515428110957f, 0.0030258141923695803f, 0.002863653702661395f, 0.0025826389901340008f, 0.0024441767018288374f, 0.002609365154057741f, 0.0029467532876878977f, 0.002448274055495858f, 0.003418843261897564f, 0.001886975602246821f, 0.002525055781006813f, 0.008315415121614933f, 0.0025032758712768555f, 0.0014930899487808347f, 0.002733937930315733f, 0.004373947158455849f, 0.005050637759268284f, 0.002205322962254286f, 0.0026183060836046934f, 0.003332538064569235f, 0.0024583153426647186f, 0.0023419861681759357f, 0.0031636375933885574f, 0.00319215701892972f, 0.0027548542711883783f, 0.002161251613870263f, 0.002393498783931136f, 0.004386791493743658f, 0.0028550606220960617f, 0.003345693228766322f, 0.004054656717926264f, 0.004500865936279297f, 0.00287587009370327f, 0.0016575068002566695f, 0.003652197541669011f, 0.002782481722533703f, 0.003305248450487852f, 0.0027362003456801176f, 0.0033595829736441374f, 0.001605078810825944f, 0.0021809260360896587f, 0.0021762712858617306f, 0.0031812693923711777f, 0.0013517213519662619f, 0.0014405368128791451f, 0.004636514466255903f, 0.003868891391903162f, 0.003210975555703044f, 0.003913572523742914f, 0.0021449285559356213f, 0.003405594266951084f, 0.002101670252159238f, 0.0033950323704630136f, 0.002489011734724045f, 0.004243303555995226f);
static const ai_layer_format_type conv2d_22_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_23_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_23_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_23_pad_before_t_in_0_shape_h_const_u32 = 14;

static const ai_u16 conv2d_23_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_23_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_23_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_23_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_23_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_23_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_23_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_23_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_23_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_23_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0020332206040620804f, 0.0023793557193130255f, 0.0022643301635980606f, 0.0026167354080826044f, 0.003233488416299224f, 0.002895829500630498f, 0.002445661462843418f, 0.002109532244503498f, 0.0023331595584750175f, 0.0029362814966589212f, 0.0029990223702043295f, 0.010517610237002373f, 0.004636521451175213f, 0.002281599212437868f, 0.003724714508280158f, 0.0023895923513919115f, 0.0017667084466665983f, 0.0014158625854179263f, 0.0020072199404239655f, 0.002622829517349601f, 0.003180492203682661f, 0.002810124773532152f, 0.0016494635492563248f, 0.002271686913445592f, 0.0013922322541475296f, 0.002986490959301591f, 0.0013173402985557914f, 0.0026790047995746136f, 0.0017397261690348387f, 0.001924196258187294f, 0.002989912638440728f, 0.0027033223304897547f, 0.0019901785999536514f, 0.0014496330404654145f, 0.002435613190755248f, 0.004277430009096861f, 0.0018849819898605347f, 0.0014873238978907466f, 0.002031932584941387f, 0.001974097453057766f, 0.004710504785180092f, 0.0022421658504754305f, 0.003286842256784439f, 0.0022619825322180986f, 0.0035055475309491158f, 0.0033513412345200777f, 0.0024431711062788963f, 0.0025169854052364826f, 0.003026634221896529f, 0.0015367123996838927f, 0.007759432774037123f, 0.0017442411044612527f, 0.0023881630040705204f, 0.0016488732071593404f, 0.002814394887536764f, 0.002654654672369361f, 0.0035661959555000067f, 0.0027470735367387533f, 0.0016699680127203465f, 0.0023354629520326853f, 0.002512863604351878f, 0.0014244230696931481f, 0.006770151201635599f, 0.0007491631549783051f, 0.0010675395606085658f, 0.0024467390030622482f, 0.002984769642353058f, 0.002826558193191886f, 0.002173065207898617f, 0.0026226574555039406f, 0.0031713657081127167f, 0.003258613171055913f, 0.002116075251251459f, 0.0018810862675309181f, 0.002352254930883646f, 0.002836637431755662f, 0.003031902713701129f, 0.0020809227135032415f, 0.0026565201114863157f, 0.0012895155232399702f, 0.0018946502823382616f, 0.002832910744473338f, 0.0025806755293160677f, 0.0021700779907405376f, 0.004350374452769756f, 0.0022582528181374073f, 0.0012946989154443145f, 0.00373109825886786f, 0.0031010680831968784f, 0.003063556971028447f, 0.0028124067466706038f, 0.0011622884776443243f, 0.0015688210260123014f, 0.002047077752649784f, 0.002644612453877926f, 0.001548680360428989f, 0.0022129903081804514f, 0.008096538484096527f, 0.003699793480336666f, 0.0023323732893913984f, 0.0015311294700950384f, 0.0103403739631176f, 0.0019476453308016062f, 0.0022100969217717648f, 0.0017198548885062337f, 0.0032032253220677376f, 0.0009748868178576231f, 0.0021107622887939215f, 0.003909499384462833f, 0.0029675052501261234f, 0.0027030983474105597f, 0.002199339447543025f, 0.004845616407692432f, 0.0030064282473176718f, 0.0036436880473047495f, 0.0019572984892874956f, 0.0011248568771407008f, 0.0024609609972685575f, 0.0020359342452138662f, 0.0034215764608234167f, 0.002598154591396451f, 0.002011190401390195f, 0.0024792132899165154f, 0.0028747639153152704f, 0.0025601033121347427f, 0.001381402718834579f, 0.0022672098129987717f, 0.002691967412829399f, 0.0030956543050706387f, 0.0014926346484571695f, 0.0015130572719499469f, 0.003435230115428567f, 0.0026365446392446756f, 0.0030804257839918137f, 0.002232462400570512f, 0.0016609992599114776f, 0.002465307479724288f, 0.0046150414273142815f, 0.0021655517630279064f, 0.0016074372688308358f, 0.001305216457694769f, 0.0022922176867723465f, 0.003282642923295498f, 0.003330341074615717f, 0.0027283933013677597f, 0.0022134785540401936f, 0.0021155928261578083f, 0.0026825398672372103f, 0.0020487860310822725f, 0.0032128398306667805f, 0.008003643713891506f, 0.0017265467904508114f, 0.007319142576307058f, 0.002752486849203706f, 0.0008441574173048139f, 0.002694136695936322f, 0.0018338888185098767f, 0.0027727538254112005f, 0.0024904455058276653f, 0.003888743929564953f, 0.0016219764947891235f, 0.0035922143142670393f, 0.0027069631032645702f, 0.001696062390692532f, 0.010310119017958641f, 0.001745150308124721f, 0.0036265659146010876f, 0.0017717587761580944f, 0.001997687853872776f, 0.001809129142202437f, 0.0018527050269767642f, 0.0038990117609500885f, 0.0029689788352698088f, 0.0019521900685504079f, 0.0018499994184821844f, 0.005378427915275097f, 0.002444326411932707f, 0.0022966142278164625f, 0.0023678066208958626f, 0.0034401111770421267f, 0.001212300849147141f, 0.0045255147852003574f, 0.001882071141153574f, 0.001586376572959125f, 0.01019772607833147f, 0.0017051328904926777f, 0.001219692756421864f, 0.0023187564220279455f, 0.002203278010711074f, 0.004195408429950476f, 0.0018643966177478433f, 0.0026738480664789677f, 0.0018996992148458958f, 0.0033584777265787125f, 0.00246840319596231f, 0.0016790935769677162f, 0.002033332595601678f, 0.00786225963383913f, 0.0036095816176384687f, 0.0029992840718477964f, 0.0017280117608606815f, 0.0027006783057004213f, 0.0024621172342449427f, 0.0022456846199929714f, 0.003063871990889311f, 0.0016728580230847f, 0.0030441866256296635f, 0.0014847510028630495f, 0.002664715051651001f, 0.0027612922713160515f, 0.0024377277586609125f, 0.0021979196462780237f, 0.001476809149608016f, 0.002254548016935587f, 0.002488404745236039f, 0.003809642745181918f, 0.005483477376401424f, 0.002868903335183859f, 0.002478968817740679f, 0.001973948674276471f, 0.0033058603294193745f, 0.0013829362578690052f, 0.005133609287440777f, 0.002019004663452506f, 0.002212580991908908f, 0.003489495487883687f, 0.0023118206299841404f, 0.0017909767339006066f, 0.003872139845043421f, 0.0015987087972462177f, 0.0016813138499855995f, 0.0038243031594902277f, 0.0021348223090171814f, 0.003942809533327818f, 0.005738256964832544f, 0.0028980583883821964f, 0.003397013060748577f, 0.001444897148758173f, 0.0019793228711932898f, 0.0024021074641495943f, 0.0029076910577714443f, 0.0026787996757775545f, 0.011938829906284809f, 0.0020380918867886066f, 0.01047068927437067f, 0.0041128285229206085f, 0.0017153745284304023f, 0.0023997193202376366f, 0.002857848536223173f, 0.0023000987712293863f, 0.004705559462308884f, 0.0033144138287752867f, 0.005663118325173855f, 0.0024750709999352694f, 0.0025812117382884026f, 0.0008207942009903491f);
static const ai_u16 conv2d_23_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_23_t_out_0_shape_h_const_u16 = 7;

static const ai_u16 conv2d_24_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_24_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_24_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_24_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_24_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_24_t_out_0_shape_ch_const_u16 = 512;
static const ai_i8 conv2d_24_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_24_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_24_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_24_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_24_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002254975028336048f, 0.005201463587582111f, 0.003162702312693f, 0.0029678463470190763f, 0.00265845051035285f, 0.002406638814136386f, 0.00260637397877872f, 0.0033085448667407036f, 0.010929005220532417f, 0.0038434683810919523f, 0.0025477383751422167f, 0.0032736340072005987f, 0.002415598137304187f, 0.002853578655049205f, 0.002860923996195197f, 0.003514921059831977f, 0.002160478848963976f, 0.0034032801631838083f, 0.002935946686193347f, 0.003599829040467739f, 0.004715177696198225f, 0.0028283833526074886f, 0.003829657332971692f, 0.003515929216518998f, 0.003457775106653571f, 0.0035734958946704865f, 0.0025791628286242485f, 0.0029244953766465187f, 0.00378073425963521f, 0.005422373302280903f, 0.0031619558576494455f, 0.004647391848266125f, 0.0036739131901413202f, 0.003071469021961093f, 0.003083721734583378f, 0.0034831410739570856f, 0.003952705767005682f, 0.004145179875195026f, 0.0007782420725561678f, 0.0035476665943861008f, 0.0030962990131229162f, 0.0033807510044425726f, 0.0037220034282654524f, 0.004247829783707857f, 0.004483914468437433f, 0.0036934176459908485f, 0.005077865906059742f, 0.002924094907939434f, 0.003720613429322839f, 0.002493270905688405f, 0.003295620670542121f, 0.0021017268300056458f, 0.0025525828823447227f, 0.00362573703750968f, 0.0029884525574743748f, 0.008175027556717396f, 0.00157557416241616f, 0.002451187465339899f, 0.004971610847860575f, 0.002570620970800519f, 0.0036793563049286604f, 0.002730736741796136f, 0.003811451606452465f, 0.002945888787508011f, 0.0022622491233050823f, 0.0022953790612518787f, 0.008205372840166092f, 0.0061932522803545f, 0.0041425530798733234f, 0.00322070624679327f, 0.004724404308944941f, 0.0027430786285549402f, 0.002616961719468236f, 0.0029126927256584167f, 0.003288951003924012f, 0.0022568479180336f, 0.002039126353338361f, 0.003327334998175502f, 0.005454554688185453f, 0.002749752951785922f, 0.0031348855700343847f, 0.002998537616804242f, 0.0027869781479239464f, 0.002156593604013324f, 0.002676381031051278f, 0.001553986920043826f, 0.0028137608896940947f, 0.00564780505374074f, 4.658981467287049e-08f, 0.0035225667525082827f, 0.0043715923093259335f, 0.002455494599416852f, 0.0034768995828926563f, 0.0034651714377105236f, 0.0062703778967261314f, 0.002165898447856307f, 0.003051469102501869f, 0.0017509434837847948f, 0.0032576473895460367f, 0.00532732205465436f, 0.0027079125866293907f, 0.009061804041266441f, 0.003263065591454506f, 0.0029886809643357992f, 0.0024700723588466644f, 0.020357219502329826f, 0.0027511068619787693f, 0.004396037198603153f, 0.0031304298900067806f, 0.0019889899995177984f, 0.005761930253356695f, 0.00429440988227725f, 0.004220279399305582f, 0.0032230992801487446f, 0.0032466575503349304f, 0.004343245644122362f, 0.002842111047357321f, 0.005577384028583765f, 0.002740842755883932f, 0.002128033200278878f, 0.003021551761776209f, 0.0023860414512455463f, 0.002686059568077326f, 0.0037475479766726494f, 0.0029338200110942125f, 0.0032536794897168875f, 0.0029333955608308315f, 0.0031592727173119783f, 0.0027317912317812443f, 0.0023629851639270782f, 0.0061882371082901955f, 0.004185381345450878f, 0.0027930480428040028f, 0.006872078869491816f, 0.003149800468236208f, 0.003733557416126132f, 0.003498723963275552f, 0.00380937778390944f, 0.0029288092628121376f, 0.0034033474512398243f, 0.004900672938674688f, 0.003429703414440155f, 0.002607014961540699f, 0.002856927691027522f, 0.006147697102278471f, 0.0040977769531309605f, 0.0035226831678301096f, 0.004049714654684067f, 0.004267693031579256f, 0.002814969513565302f, 0.003322665812447667f, 0.004020156338810921f, 0.004046782851219177f, 0.005246869754046202f, 0.0040374998934566975f, 0.0034277590457350016f, 0.0035725801717489958f, 0.003307913662865758f, 0.01985706388950348f, 0.0029572718776762486f, 0.0039095282554626465f, 0.0034565741661936045f, 0.004174709320068359f, 0.004012380260974169f, 0.004284415394067764f, 0.002464012475684285f, 0.0028023957274854183f, 0.004743558820337057f, 0.0033135367557406425f, 0.0033843698911368847f, 0.00471592228859663f, 0.003678404726088047f, 0.004019869491457939f, 0.0036188557278364897f, 0.0048239571042358875f, 0.0033791856840252876f, 0.00333982240408659f, 0.0030133454129099846f, 0.004578045569360256f, 0.0033723018132150173f, 0.0035183841828256845f, 0.009203318506479263f, 0.004161651246249676f, 0.00541981915012002f, 0.0018527884967625141f, 0.00409199483692646f, 0.004248466342687607f, 0.006363118067383766f, 0.003337634028866887f, 0.006734040565788746f, 0.0031878268346190453f, 0.003061298979446292f, 0.004053629003465176f, 0.0021193181164562702f, 0.003247359534725547f, 0.004733114968985319f, 0.001411007484421134f, 0.0033093630336225033f, 0.0032246552873402834f, 0.0019333168165758252f, 0.002782172756269574f, 0.0041554090566933155f, 0.0074477107264101505f, 0.003398977220058441f, 0.003234461648389697f, 0.0027236088644713163f, 0.0059577301144599915f, 0.0018206527456641197f, 0.004228556994348764f, 0.0030819731764495373f, 0.0036447769962251186f, 0.0034267755690962076f, 0.002468131948262453f, 0.002099628560245037f, 0.00539637915790081f, 0.0032509444281458855f, 0.0020346115343272686f, 0.004339140839874744f, 0.0034675118513405323f, 0.002530170138925314f, 0.002920013153925538f, 0.002951378934085369f, 0.002889134455472231f, 0.005162553861737251f, 0.0027378923259675503f, 0.002828414086252451f, 0.002305331639945507f, 0.0032534352503716946f, 0.0022582875099033117f, 0.0036232047714293003f, 0.004135597962886095f, 0.004916520789265633f, 0.002822559792548418f, 0.005282723810523748f, 0.002955856267362833f, 0.003587351180613041f, 0.0026491517201066017f, 0.0032620076090097427f, 0.004389527719467878f, 0.004789413418620825f, 0.0036936637479811907f, 0.0023215599358081818f, 0.0031111848074942827f, 0.00913339201360941f, 0.004379064776003361f, 0.0034888959489762783f, 0.004817350767552853f, 0.0029136028606444597f, 0.0032871761359274387f, 0.00390815082937479f, 0.005843651946634054f, 0.0077410051599144936f, 0.003755182260647416f, 0.004369111265987158f, 0.0032761418260633945f, 0.003156298538669944f, 0.003568535204976797f, 0.002209492726251483f, 0.0036249817349016666f, 0.0032461665105074644f, 0.005265092011541128f, 0.0025633820332586765f, 0.002457286464050412f, 0.002597157144919038f, 0.0036329010035842657f, 0.0024657887406647205f, 0.0033020589035004377f, 0.0032027773559093475f, 0.0028236620128154755f, 0.0030067823827266693f, 0.003380816662684083f, 0.0018804382998496294f, 0.003132009878754616f, 0.006248273886740208f, 0.00443690363317728f, 0.0030888495966792107f, 0.002384497318416834f, 0.0018107283394783735f, 0.002795064589008689f, 0.004210344050079584f, 0.008110655471682549f, 0.0019998112693428993f, 0.0010948659619316459f, 0.0031335661187767982f, 0.0018275465117767453f, 0.002801831578835845f, 0.004645322449505329f, 0.003109515178948641f, 0.002307855524122715f, 0.0036155632697045803f, 0.002165361074730754f, 0.0029829153791069984f, 0.012281086295843124f, 0.0016323477029800415f, 0.003693888196721673f, 0.004950491711497307f, 0.003851192770525813f, 0.003243295243009925f, 0.0018771657487377524f, 0.003346837591379881f, 0.0025219384115189314f, 0.0036776612978428602f, 0.0042615607380867004f, 0.0031564736273139715f, 0.0016218493692576885f, 0.005237867124378681f, 0.0031775902025401592f, 0.002986358944326639f, 0.003464530222117901f, 0.003894699504598975f, 0.002683552447706461f, 0.003292984329164028f, 0.009368515573441982f, 0.001972037833184004f, 0.0029527400620281696f, 0.003961695358157158f, 0.004154103342443705f, 0.0035960362292826176f, 0.0027450029738247395f, 0.003373224288225174f, 0.005189936142414808f, 0.003945626318454742f, 0.002582044107839465f, 0.002175693865865469f, 0.003273661946877837f, 0.0036232394631952047f, 0.0023573932703584433f, 0.0035149764735251665f, 0.002833459060639143f, 0.005532985553145409f, 0.01807096041738987f, 0.004054413177073002f, 0.005974913947284222f, 0.0026227182243019342f, 0.002886159112676978f, 0.0036038830876350403f, 0.0017813128652051091f, 0.0021564755588769913f, 0.003231288632377982f, 0.0032086221035569906f, 0.0031415026169270277f, 0.00436453428119421f, 0.002843251219019294f, 0.005648361053317785f, 0.0028642427641898394f, 0.002525724470615387f, 0.0021412933710962534f, 0.0032850150018930435f, 0.0028038760647177696f, 0.003148541087284684f, 0.005209553986787796f, 0.0020632045343518257f, 0.003438466228544712f, 0.0024911004584282637f, 0.004162490833550692f, 0.0033778368961066008f, 0.0045056832022964954f, 0.003215360688045621f, 0.0030536458361893892f, 0.006516134832054377f, 0.003719025757163763f, 0.003927864599972963f, 0.0031786542385816574f, 0.0033999027218669653f, 0.0009075154084712267f, 0.0029715446289628744f, 0.0029544206336140633f, 0.003981201909482479f, 0.005690590478479862f, 0.003774095792323351f, 0.0016591320745646954f, 0.00344489561393857f, 0.004166748374700546f, 0.0014153716620057821f, 0.0017434734618291259f, 0.003685158211737871f, 0.003571405541151762f, 0.003068007528781891f, 0.004525003954768181f, 0.0038658762350678444f, 0.004811877850443125f, 0.002694582799449563f, 0.004739161115139723f, 0.0036816734354943037f, 0.003444015048444271f, 0.0024840713012963533f, 0.002428915351629257f, 0.001837403979152441f, 0.0032573817297816277f, 0.0021167967934161425f, 0.0030958650168031454f, 0.0032821837812662125f, 0.003580875229090452f, 0.004817291162908077f, 0.0028809397481381893f, 0.0024368176236748695f, 0.004578272346407175f, 0.00461178133264184f, 0.0030982985626906157f, 0.003085474716499448f, 0.0032515758648514748f, 0.002824334427714348f, 0.004613700322806835f, 0.0037282719276845455f, 0.002342867199331522f, 0.0027741820085793734f, 0.0024880310520529747f, 0.0034261669497936964f, 0.003544706152752042f, 0.004841670859605074f, 0.0031094977166503668f, 0.0029334237333387136f, 0.002459276467561722f, 0.004357732832431793f, 0.0037614020984619856f, 0.0029638721607625484f, 0.002580834785476327f, 0.006422574631869793f, 0.005424788687378168f, 0.0022626991849392653f, 0.0020998972468078136f, 0.0037950912956148386f, 0.0044023399241268635f, 0.004070261958986521f, 0.0019067534012719989f, 0.00294551532715559f, 0.0036553337704390287f, 0.005353300366550684f, 0.003931647166609764f, 0.004059078637510538f, 0.003539603902027011f, 0.0028911603149026632f, 0.019946323707699776f, 0.005368695128709078f, 0.0018156307050958276f, 0.002832521451637149f, 0.005132297053933144f, 0.0036431769840419292f, 0.002664596773684025f, 0.0037814988754689693f, 0.00420306995511055f, 0.0038086639251559973f, 0.0059918626211583614f, 0.002379149664193392f, 0.0020304122008383274f, 0.003071689046919346f, 0.0021619501058012247f, 0.001513672643341124f, 0.0025835775304585695f, 0.005510264076292515f, 0.002849053591489792f, 0.0033753931056708097f, 0.006713388953357935f, 0.004668319597840309f, 0.005500535015016794f, 0.0025863240007311106f, 0.003312988905236125f, 0.001277951174415648f, 0.0042063575237989426f, 0.0025633934419602156f, 0.005097464192658663f, 0.0037803356535732746f, 0.0034586985129863024f, 0.0032236420083791018f, 0.003310962812975049f, 0.002554320963099599f, 0.005299944430589676f, 0.0032098719384521246f, 0.004307556431740522f, 0.003227046225219965f, 0.0025109313428401947f, 0.0035806363448500633f, 0.004787189420312643f, 0.0032815723679959774f, 0.014196764677762985f, 0.0021281386725604534f, 0.0028885838109999895f, 0.00204260041937232f, 0.004572899546474218f, 0.0032139301765710115f, 0.004587194416671991f, 0.004677042830735445f, 0.005185638554394245f, 0.004436163231730461f, 0.0028184843249619007f, 0.0026237759739160538f, 0.002604832174256444f, 0.0026644975878298283f, 0.0030359860975295305f, 0.0036835933569818735f, 0.0040660761296749115f, 0.005588154308497906f, 0.0037809472996741533f, 0.0054262843914330006f, 0.002976395422592759f, 0.0025344383902847767f, 0.005402779206633568f, 0.003912142477929592f, 0.0035836210008710623f, 0.003073020139709115f, 0.002753985347226262f, 0.002723683137446642f, 0.006345645058900118f, 0.0038682143203914165f, 0.0037773605436086655f, 0.0038147869054228067f, 0.006504612974822521f, 0.003969627432525158f, 0.0032749036327004433f, 0.0039481548592448235f, 0.002495633438229561f, 0.003852996276691556f);
static const ai_layer_format_type conv2d_24_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_25_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_25_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_25_pad_before_t_in_0_shape_h_const_u32 = 7;

static const ai_u16 conv2d_25_t_in_0_shape_w_const_u16 = 9;
static const ai_u16 conv2d_25_t_in_0_shape_h_const_u16 = 9;
static const ai_u16 conv2d_25_t_in_0_shape_ch_const_u16 = 512;
static const ai_u16 conv2d_25_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_25_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_25_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_25_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_25_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_25_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_25_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.009767884388566017f, 0.0028761927969753742f, 0.004277885891497135f, 0.004079695325344801f, 0.0030360000673681498f, 0.010145056992769241f, 0.0032938236836344004f, 0.0042476961389184f, 0.002832723781466484f, 0.011429362930357456f, 0.007390881888568401f, 0.0031223308760672808f, 0.0044386121444404125f, 0.004646616987884045f, 0.008493809960782528f, 0.003524418454617262f, 0.012589482590556145f, 0.0023879590444266796f, 0.008670646697282791f, 0.004370694514364004f, 0.0031247877050191164f, 0.00317548424936831f, 0.0036355440970510244f, 0.010040033608675003f, 0.002815789543092251f, 0.009144256822764874f, 0.009977588430047035f, 0.004758563358336687f, 0.004017939325422049f, 0.002550050849094987f, 0.004070667549967766f, 0.03440234437584877f, 0.00279394187964499f, 0.0041139149107038975f, 0.0034836260601878166f, 0.0024441038258373737f, 0.005328523460775614f, 0.004721959121525288f, 0.01650252565741539f, 0.0042923553846776485f, 0.003597309347242117f, 0.007684383541345596f, 0.002883289707824588f, 0.005760673899203539f, 0.00919972825795412f, 0.006655299104750156f, 0.002734559588134289f, 0.0073175798170268536f, 0.0027362750843167305f, 0.0036473635118454695f, 0.0029028209391981363f, 0.005575112067162991f, 0.00795664917677641f, 0.0039772698655724525f, 0.004035088699311018f, 0.00572185730561614f, 0.013045067898929119f, 0.0028293386567384005f, 0.013370123691856861f, 0.006873113568872213f, 0.0042863693088293076f, 0.03741098940372467f, 0.003360939212143421f, 0.004775193054229021f, 0.016695257276296616f, 0.013975112698972225f, 0.0030055330134928226f, 0.0026890039443969727f, 0.004662071354687214f, 0.004004386719316244f, 0.0018276798073202372f, 0.004672764800488949f, 0.004166297148913145f, 0.004770689643919468f, 0.00285144941881299f, 0.013805369846522808f, 0.012360216118395329f, 0.004895409103482962f, 0.0034397642593830824f, 0.00536379124969244f, 0.004316852893680334f, 0.020763371139764786f, 0.010718615725636482f, 0.010311877354979515f, 0.005669021978974342f, 0.010798227041959763f, 0.004741448909044266f, 0.0026878248900175095f, 0.09200485795736313f, 0.016335075721144676f, 0.006848050281405449f, 0.005167052615433931f, 0.003920430317521095f, 0.003312953282147646f, 0.003050016239285469f, 0.005016924813389778f, 0.0032382311765104532f, 0.01381958369165659f, 0.0035721012391149998f, 0.003156139748170972f, 0.007673189509660006f, 0.0048892139457166195f, 0.002980016404762864f, 0.0034059318713843822f, 0.007596956100314856f, 0.005603509023785591f, 0.0063720629550516605f, 0.007093079388141632f, 0.002439522184431553f, 0.012822277843952179f, 0.003080871654674411f, 0.0025620851665735245f, 0.0033936069812625647f, 0.004653708077967167f, 0.0032029091380536556f, 0.002944919280707836f, 0.00863791722804308f, 0.002230199286714196f, 0.011090652085840702f, 0.011537973769009113f, 0.002937242854386568f, 0.006051565520465374f, 0.008947530761361122f, 0.0035759969614446163f, 0.00461543770506978f, 0.015719860792160034f, 0.004930716473609209f, 0.005739428102970123f, 0.008049939759075642f, 0.01640177331864834f, 0.0026905799750238657f, 0.0022808657959103584f, 0.00982640404254198f, 0.011077585630118847f, 0.0034362482838332653f, 0.005011241417378187f, 0.010858106426894665f, 0.0038128704763948917f, 0.004166454542428255f, 0.00358559750020504f, 0.0051102470606565475f, 0.005545724183320999f, 0.003490412374958396f, 0.003919564187526703f, 0.002185906982049346f, 0.004209903534501791f, 0.002643495798110962f, 0.0030762648675590754f, 0.041313376277685165f, 0.004945305176079273f, 0.0036087355110794306f, 0.0028818657156080008f, 0.0076668839901685715f, 0.005040230695158243f, 0.0040540085174143314f, 0.008349153213202953f, 0.0033072009682655334f, 0.005092789884656668f, 0.0015611705603078008f, 0.011237381026148796f, 0.0075556267984211445f, 0.007335699163377285f, 0.005401525180786848f, 0.00285113207064569f, 0.004476029425859451f, 0.009586161002516747f, 0.003706075483933091f, 0.005974049214273691f, 0.00358956097625196f, 0.008558856323361397f, 0.002957031363621354f, 0.003105665324255824f, 0.00908847525715828f, 0.0025095136370509863f, 0.0026723979972302914f, 0.005955476313829422f, 0.0037453104741871357f, 0.005529071670025587f, 0.0031351950019598007f, 0.01693127304315567f, 0.003942552488297224f, 0.004536113236099482f, 0.004359155427664518f, 0.004722136072814465f, 0.010401900857686996f, 0.001859965268522501f, 0.0039549027569592f, 0.0037464844062924385f, 0.0019927741959691048f, 0.0019697006791830063f, 0.004206622950732708f, 0.0051633939146995544f, 0.0025533344596624374f, 0.005672863684594631f, 0.004471449181437492f, 0.0026936668436974287f, 0.017562221735715866f, 0.004891987424343824f, 0.0038578754756599665f, 0.009452632628381252f, 0.0035409489646553993f, 0.011125669814646244f, 0.0089584831148386f, 0.0032128652092069387f, 0.003300325945019722f, 0.005543134640902281f, 0.002178166527301073f, 0.013012240640819073f, 0.0028443618211895227f, 0.004196700640022755f, 0.0025298227556049824f, 0.004066926427185535f, 0.01254526898264885f, 0.028011495247483253f, 0.007643936201930046f, 0.004676578566431999f, 0.013762234710156918f, 0.002605161629617214f, 0.003481368301436305f, 0.003456059144809842f, 0.0023673060350120068f, 0.005962053779512644f, 0.004359073005616665f, 0.002682532649487257f, 0.004066431429237127f, 0.013325325213372707f, 0.009832784533500671f, 0.0027738893404603004f, 0.01128165889531374f, 0.003549225628376007f, 0.004301265347748995f, 0.009384719654917717f, 0.003482518484815955f, 0.002913125092163682f, 0.0044863116927444935f, 0.0027498833369463682f, 0.004224108997732401f, 0.003260010154917836f, 0.002971945097669959f, 0.010229967534542084f, 0.008419224061071873f, 0.03915590047836304f, 0.003873384790495038f, 0.004681306891143322f, 0.017440220341086388f, 0.00363220670260489f, 0.009355548769235611f, 0.004207043442875147f, 0.0037030891980975866f, 0.0030908819753676653f, 0.0027081784792244434f, 0.014747626148164272f, 0.002150182379409671f, 0.013627571985125542f, 0.004306183662265539f, 0.00989890843629837f, 0.0025549784768372774f, 0.017273886129260063f, 0.003575177164748311f, 0.00406366353854537f, 0.004014989826828241f, 0.012289019301533699f, 0.017787734046578407f, 0.008340587839484215f, 0.005770213436335325f, 0.009775056503713131f, 0.008723629638552666f, 0.02164049632847309f, 0.010108651593327522f, 0.08046311885118484f, 0.0029084337875247f, 0.010572275146842003f, 0.0037247231230139732f, 0.002225971082225442f, 0.0037966021336615086f, 0.003696083091199398f, 0.0051682149060070515f, 0.008615599013864994f, 0.00472619105130434f, 0.003159482730552554f, 0.003184068016707897f, 0.00487601850181818f, 0.023268237709999084f, 0.006733002606779337f, 0.005032326560467482f, 0.007805389352142811f, 0.0033849915489554405f, 0.0024189436808228493f, 0.010893073864281178f, 0.0032971242908388376f, 0.0058715008199214935f, 0.003466425696387887f, 0.003011871362105012f, 0.012568345293402672f, 0.0056611355394124985f, 0.002444143872708082f, 0.003919977694749832f, 0.011461637914180756f, 0.011952816508710384f, 0.006335313897579908f, 0.003886688267812133f, 0.004031098447740078f, 0.004358153324574232f, 0.0032954670023173094f, 0.013074835762381554f, 0.0022823710460215807f, 0.022496618330478668f, 0.007824352942407131f, 0.002990343142300844f, 0.0022279375698417425f, 0.0045405165292322636f, 0.00366168562322855f, 0.002400055294856429f, 0.006745601072907448f, 0.002879899926483631f, 0.002941161161288619f, 0.0041928356513381f, 0.0036797323264181614f, 0.005210116971284151f, 0.008863178081810474f, 0.00281374529004097f, 0.002158326329663396f, 0.009482668712735176f, 0.011713765561580658f, 0.00497437035664916f, 0.002479317132383585f, 0.01830611377954483f, 0.001909639802761376f, 0.0025887498632073402f, 0.003462414722889662f, 0.00603172043338418f, 0.005027917213737965f, 0.00234016589820385f, 0.008416342549026012f, 0.010893660597503185f, 0.0035063116811215878f, 0.02277088165283203f, 0.009485515765845776f, 0.0043066637590527534f, 0.0039015107322484255f, 0.0030585722997784615f, 0.004175303038209677f, 0.010063495486974716f, 0.004329251125454903f, 0.004652082454413176f, 0.0212558601051569f, 0.004479782190173864f, 0.005247452296316624f, 0.003952000290155411f, 0.00464885588735342f, 0.0027704241219908f, 0.027805736288428307f, 0.004132171627134085f, 0.0049508968368172646f, 0.005005417857319117f, 0.0027220910415053368f, 0.0031092206481844187f, 0.003453976009041071f, 0.007330466061830521f, 0.0034917937591671944f, 0.003979439847171307f, 0.0031100416090339422f, 0.005244679283350706f, 0.015725696459412575f, 0.023544585332274437f, 0.0072310869581997395f, 0.004691045731306076f, 0.00261047319509089f, 0.0033851969055831432f, 0.004022945184260607f, 0.011079473420977592f, 0.005321632605046034f, 0.0032199008855968714f, 0.006868808995932341f, 0.013616475276648998f, 0.0028473902493715286f, 0.004230034537613392f, 0.004412551876157522f, 0.004044375382363796f, 0.0036672106944024563f, 0.0023136467207223177f, 0.004239263013005257f, 0.004022729583084583f, 0.003357687732204795f, 0.0032078467775136232f, 0.004595380276441574f, 0.007950789295136929f, 0.012904888018965721f, 0.004652619361877441f, 0.013846935704350471f, 0.011155379936099052f, 0.003027970204129815f, 0.004397413227707148f, 0.0022012630943208933f, 0.003001956269145012f, 0.009286383166909218f, 0.005261412356048822f, 0.002965095918625593f, 0.003625327255576849f, 0.0034677458461374044f, 0.006317431572824717f, 0.012225975282490253f, 0.0037577450275421143f, 0.0031612995080649853f, 0.02718684822320938f, 0.002679506316781044f, 0.008578309789299965f, 0.003006979124620557f, 0.0034225517883896828f, 0.0027265679091215134f, 0.004532326944172382f, 0.009275243617594242f, 0.004962112754583359f, 0.007912339642643929f, 0.002689995802938938f, 0.0036930751521140337f, 0.004987637046724558f, 0.002722252393141389f, 0.0032075049821287394f, 0.008192823268473148f, 0.010093298740684986f, 0.0035998576786369085f, 0.0033720682840794325f, 0.008179056458175182f, 0.009104830212891102f, 0.004079854115843773f, 0.0034781116992235184f, 0.003829144174233079f, 0.006397263612598181f, 0.004502154886722565f, 0.0038077293429523706f, 0.004709139000624418f, 0.005234741140156984f, 0.003144980175420642f, 0.004960255231708288f, 0.003582226810976863f, 0.002483961172401905f, 0.0030768699944019318f, 0.004661303013563156f, 0.004162191413342953f, 0.004173281602561474f, 0.0023863145615905523f, 0.0028984593227505684f, 0.01005072146654129f, 0.012289199978113174f, 0.008001682348549366f, 0.01077982410788536f, 0.009766845963895321f, 0.004715722054243088f, 0.00230152765288949f, 0.004944867920130491f, 0.0035404013469815254f, 0.001688795629888773f, 0.004640149883925915f, 0.002376482356339693f, 0.006501862779259682f, 0.0151545200496912f, 0.021652331575751305f, 0.003207877976819873f, 0.005425340496003628f, 0.002246099291369319f, 0.0027193103451281786f, 0.0024619705509394407f, 0.004244260955601931f, 0.002263907575979829f, 0.004986870102584362f, 0.0032288075890392065f, 0.004570506513118744f, 0.003468872047960758f, 0.0037882844917476177f, 0.004335225559771061f, 0.00383444270119071f, 0.003132116049528122f, 0.0028840808663517237f, 0.002352190436795354f, 0.010900850407779217f, 0.0035335158463567495f, 0.0060238041914999485f, 0.0033064759336411953f, 0.00400112709030509f, 0.0032714977860450745f, 0.003820305922999978f, 0.00287212198600173f, 0.002772655338048935f, 0.004319825675338507f, 0.0033080291468650103f, 0.00555013632401824f, 0.009801006875932217f, 0.0036272036377340555f, 0.0021399410907179117f, 0.0023830062709748745f, 0.005211171228438616f, 0.0032837982289493084f, 0.008232209831476212f, 0.0026302174665033817f, 0.00791621208190918f, 0.0027590638492256403f, 0.002658628858625889f, 0.0037122941575944424f, 0.003306381870061159f, 0.003100005676969886f, 0.005382345523685217f, 0.0033209286630153656f, 0.002913476899266243f, 0.004236305132508278f, 0.0033486841712146997f, 0.003327316138893366f, 0.002220651600509882f, 0.003213243093341589f, 0.00400827219709754f, 0.004801888484507799f, 0.004973011091351509f);
static const ai_u16 conv2d_25_t_out_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_25_t_out_0_shape_h_const_u16 = 7;

static const ai_u16 conv2d_26_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_26_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_26_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_26_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_26_t_in_0_shape_ch_const_u16 = 512;
static const ai_u16 conv2d_26_t_out_0_shape_ch_const_u16 = 512;
static const ai_i8 conv2d_26_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_26_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_26_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_26_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_26_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.019301289692521095f, 0.013183225877583027f, 0.01752176322042942f, 0.010987472720444202f, 0.01421151589602232f, 0.020446371287107468f, 0.019732454791665077f, 0.014955947175621986f, 0.011301454156637192f, 0.01382445264607668f, 0.01103775855153799f, 0.016364891082048416f, 0.014469797722995281f, 0.01889803074300289f, 0.025058550760149956f, 0.016638316214084625f, 0.0153609374538064f, 0.013122369535267353f, 0.014961360022425652f, 0.013731143437325954f, 0.012849969789385796f, 0.01901835948228836f, 0.04846979305148125f, 0.014495895244181156f, 0.0200561061501503f, 0.014941069297492504f, 0.013872500509023666f, 0.014388734474778175f, 0.011569934897124767f, 0.014381024986505508f, 0.017144834622740746f, 0.014120683073997498f, 0.011591418646275997f, 0.01825188659131527f, 0.011223074980080128f, 0.019164923578500748f, 0.011338241398334503f, 0.024168716743588448f, 0.02607625536620617f, 0.01262515876442194f, 0.015747027471661568f, 0.01547762006521225f, 0.015232297591865063f, 0.015683388337492943f, 0.010430763475596905f, 0.014979135245084763f, 0.013035264797508717f, 0.013116108253598213f, 0.012691943906247616f, 0.011771420016884804f, 0.01777930185198784f, 0.013681218959391117f, 0.017009997740387917f, 0.013748151250183582f, 0.017686551436781883f, 0.023935385048389435f, 0.014089743606746197f, 0.01276195514947176f, 0.01270509697496891f, 0.016120504587888718f, 0.02256186120212078f, 0.013290644623339176f, 0.017254652455449104f, 0.02331058494746685f, 0.015600168146193027f, 0.0136876804754138f, 0.018198449164628983f, 0.010999025776982307f, 0.02651173248887062f, 0.009696551598608494f, 0.010388515889644623f, 0.01550743356347084f, 0.011656480841338634f, 0.012698131613433361f, 0.013638356700539589f, 0.015369553118944168f, 0.018018942326307297f, 0.020771833136677742f, 0.028680145740509033f, 0.013019264675676823f, 0.014340159483253956f, 0.018856018781661987f, 0.012690582312643528f, 0.012636483646929264f, 0.01186628919094801f, 0.014913512393832207f, 0.02538716234266758f, 0.011844528838992119f, 0.015355998650193214f, 0.013806705363094807f, 0.01190976519137621f, 0.011614345945417881f, 0.012331628240644932f, 0.011776686646044254f, 0.012115397490561008f, 0.012404786422848701f, 0.011699074879288673f, 0.027371585369110107f, 0.016497543081641197f, 0.01244112104177475f, 0.012576698325574398f, 0.01392381638288498f, 0.013791793957352638f, 0.024486945942044258f, 0.015173885971307755f, 0.0131496237590909f, 0.014969786629080772f, 0.014733054675161839f, 0.011656833812594414f, 0.012004072777926922f, 0.011803710833191872f, 0.011616590432822704f, 0.013902456499636173f, 0.01291612908244133f, 0.012239665724337101f, 0.02080133929848671f, 0.010517021641135216f, 0.014837037771940231f, 0.01359074842184782f, 0.01713624782860279f, 0.009299465455114841f, 0.015130490064620972f, 0.014765078201889992f, 0.009918059222400188f, 0.011835194192826748f, 0.01731174997985363f, 0.01843821443617344f, 0.012875576503574848f, 0.012318660505115986f, 0.017554938793182373f, 0.01423831656575203f, 0.011444071307778358f, 0.016403311863541603f, 0.016423214226961136f, 0.013078544288873672f, 0.016802502796053886f, 0.023301003500819206f, 0.014589088968932629f, 0.01525677740573883f, 0.014685772359371185f, 0.0129737863317132f, 0.01787816733121872f, 0.011898914352059364f, 0.02828698419034481f, 0.018024059012532234f, 0.012273912318050861f, 0.017882708460092545f, 0.01651197113096714f, 0.01311282254755497f, 0.012590453960001469f, 0.012771992944180965f, 0.012027557007968426f, 0.01762200891971588f, 0.01470195408910513f, 0.014413690194487572f, 0.013656324706971645f, 0.014956125058233738f, 0.015417762100696564f, 0.012202362529933453f, 0.013775686733424664f, 0.013375049456954002f, 0.01855216547846794f, 0.013291968032717705f, 0.012851765379309654f, 0.01492193154990673f, 0.011805710382759571f, 0.0131243159994483f, 0.01712038740515709f, 0.018210090696811676f, 0.014894862659275532f, 0.012715088203549385f, 0.014677763916552067f, 0.012869628146290779f, 0.012850138358771801f, 0.012844841927289963f, 0.014809840358793736f, 0.01668141968548298f, 0.0132749630138278f, 0.011026546359062195f, 0.017026470974087715f, 0.031964413821697235f, 0.016775047406554222f, 0.012550776824355125f, 0.01565335877239704f, 0.014931830577552319f, 0.01155856903642416f, 0.012740137986838818f, 0.012950429692864418f, 0.011725726537406445f, 0.013996923342347145f, 0.030474428087472916f, 0.014918568544089794f, 0.012348040007054806f, 0.014996889047324657f, 0.013242117129266262f, 0.0169226061552763f, 0.01573331467807293f, 0.013301439583301544f, 0.01025070995092392f, 0.02019556425511837f, 0.016726115718483925f, 0.014163604937493801f, 0.009480167180299759f, 0.014185307547450066f, 0.01244288869202137f, 0.015531363897025585f, 0.017146151512861252f, 0.01555656548589468f, 0.01110629178583622f, 0.014752398245036602f, 0.013226374983787537f, 0.014381320215761662f, 0.016648631542921066f, 0.013261252082884312f, 0.011636200360953808f, 0.02342609129846096f, 0.019697507843375206f, 0.020943105220794678f, 0.011692776344716549f, 0.015129007399082184f, 0.012782491743564606f, 0.01076317485421896f, 0.01815931685268879f, 0.015984825789928436f, 0.011495308019220829f, 0.01154059823602438f, 0.013231543824076653f, 0.012174933217465878f, 0.013121386058628559f, 0.012955540791153908f, 0.013083144091069698f, 0.013469535857439041f, 0.011714167892932892f, 0.012197041884064674f, 0.012078842148184776f, 0.019554248079657555f, 0.013503873720765114f, 0.022937024012207985f, 0.017363717779517174f, 0.01458101999014616f, 0.02615303546190262f, 0.014486141502857208f, 0.013202864676713943f, 0.013610686175525188f, 0.011243694461882114f, 0.01067421119660139f, 0.01220385730266571f, 0.009703702293336391f, 0.017895469442009926f, 0.011109047569334507f, 0.014572927728295326f, 0.017456546425819397f, 0.013702474534511566f, 0.021870188415050507f, 0.02407819963991642f, 0.019080311059951782f, 0.011695213615894318f, 0.01570771262049675f, 0.018590258434414864f, 0.016414960846304893f, 0.012839015573263168f, 0.01300161611288786f, 0.013066441752016544f, 0.01626691408455372f, 0.018203089013695717f, 0.023000478744506836f, 0.015819493681192398f, 0.010843662545084953f, 0.01807072013616562f, 0.014656723476946354f, 0.012126232497394085f, 0.013239210471510887f, 0.011278041638433933f, 0.014654968865215778f, 0.01778704673051834f, 0.013735814020037651f, 0.012042135931551456f, 0.015223074704408646f, 0.01965913735330105f, 0.014116739854216576f, 0.017316212877631187f, 0.015904230996966362f, 0.012601672671735287f, 0.02270379848778248f, 0.017273763194680214f, 0.011547801084816456f, 0.024305373430252075f, 0.01854027435183525f, 0.014996662735939026f, 0.011158705689013004f, 0.032106414437294006f, 0.013736088760197163f, 0.019376758486032486f, 0.012079994194209576f, 0.016665037721395493f, 0.012836058624088764f, 0.01603301241993904f, 0.022957459092140198f, 0.01723547838628292f, 0.012320470064878464f, 0.01466367021203041f, 0.02406478486955166f, 0.018037017434835434f, 0.028921613469719887f, 0.013250084593892097f, 0.019670657813549042f, 0.014123241417109966f, 0.014058353379368782f, 0.014888999052345753f, 0.014574586413800716f, 0.01345424447208643f, 0.024976098909974098f, 0.011505029164254665f, 0.015327162109315395f, 0.012513182125985622f, 0.01614386960864067f, 0.01310104038566351f, 0.026303663849830627f, 0.023691078647971153f, 0.011193180456757545f, 0.011713830754160881f, 0.013178515248000622f, 0.013189379125833511f, 0.017445959150791168f, 0.01754370704293251f, 0.021167419850826263f, 0.01801728643476963f, 0.012029158882796764f, 0.01183044072240591f, 0.026334384456276894f, 0.012466327287256718f, 0.02610906958580017f, 0.013990088365972042f, 0.01354957651346922f, 0.02185704931616783f, 0.013760266825556755f, 0.0193733312189579f, 0.012180596590042114f, 0.011531520634889603f, 0.010557444766163826f, 0.011409102939069271f, 0.014205931685864925f, 0.020896373316645622f, 0.020993035286664963f, 0.012945538386702538f, 0.014729772694408894f, 0.03260622173547745f, 0.010929565876722336f, 0.011065487749874592f, 0.01253882423043251f, 0.014798441901803017f, 0.013878410682082176f, 0.011844314634799957f, 0.010763632133603096f, 0.011168927885591984f, 0.025400135666131973f, 0.011838432401418686f, 0.010821430943906307f, 0.018628757447004318f, 0.013610213063657284f, 0.022331763058900833f, 0.010404100641608238f, 0.011979793198406696f, 0.019325997680425644f, 0.01313235517591238f, 0.012494474649429321f, 0.015222554095089436f, 0.011442885734140873f, 0.01573200151324272f, 0.013982094824314117f, 0.013365001417696476f, 0.01446054968982935f, 0.011594551615417004f, 0.013129645958542824f, 0.013292807154357433f, 0.01768365688621998f, 0.01602146029472351f, 0.01686360128223896f, 0.013739421032369137f, 0.01124146580696106f, 0.01265603955835104f, 0.015303418971598148f, 0.011209170334041119f, 0.021296760067343712f, 0.012095512822270393f, 0.011609982699155807f, 0.03062465228140354f, 0.01272168755531311f, 0.021439887583255768f, 0.01420675776898861f, 0.012128547765314579f, 0.011565310880541801f, 0.014308075420558453f, 0.0108345253393054f, 0.01456373743712902f, 0.014200464822351933f, 0.013666292652487755f, 0.01393897645175457f, 0.015349607914686203f, 0.01961704157292843f, 0.012757955119013786f, 0.011771094985306263f, 0.023199040442705154f, 0.016028834506869316f, 0.013971151784062386f, 0.02340354025363922f, 0.016408056020736694f, 0.01254833023995161f, 0.01871098391711712f, 0.011707575991749763f, 0.01390710100531578f, 0.016547448933124542f, 0.02122575417160988f, 0.015297987498342991f, 0.02227325178682804f, 0.011520514264702797f, 0.01801432855427265f, 0.012081943452358246f, 0.018409855663776398f, 0.013684266246855259f, 0.015493099577724934f, 0.015904638916254044f, 0.01524000708013773f, 0.01343114860355854f, 0.014215984381735325f, 0.013036650605499744f, 0.0149186160415411f, 0.01690356619656086f, 0.011244106106460094f, 0.014269406907260418f, 0.014436635188758373f, 0.015506291761994362f, 0.010406914167106152f, 0.012235751375555992f, 0.014690243639051914f, 0.013393498957157135f, 0.027972497045993805f, 0.01302083395421505f, 0.01957126148045063f, 0.010187150910496712f, 0.012098649516701698f, 0.015954894945025444f, 0.013204723596572876f, 0.01225288026034832f, 0.01715862937271595f, 0.04135614633560181f, 0.019144417718052864f, 0.016137804836034775f, 0.013618884608149529f, 0.017362544313073158f, 0.021184025332331657f, 0.017736123874783516f, 0.015010712668299675f, 0.014760106801986694f, 0.015513730235397816f, 0.012863855808973312f, 0.012087835930287838f, 0.013597054407000542f, 0.01004041638225317f, 0.01376627292484045f, 0.018809719011187553f, 0.01266184076666832f, 0.0115230493247509f, 0.019991299137473106f, 0.01578870788216591f, 0.01141870766878128f, 0.012256824411451817f, 0.013145234435796738f, 0.01459802221506834f, 0.01703362539410591f, 0.016058914363384247f, 0.014338393695652485f, 0.013197999447584152f, 0.0157987829297781f, 0.014090154320001602f, 0.021093593910336494f, 0.027918074280023575f, 0.014048592187464237f, 0.0237672571092844f, 0.013538307510316372f, 0.011596533469855785f, 0.013079957105219364f, 0.013716449029743671f, 0.015697181224822998f, 0.015968725085258484f, 0.017190681770443916f, 0.011589379981160164f, 0.014164083637297153f, 0.015223468653857708f, 0.029757721349596977f, 0.013331219553947449f, 0.01637262850999832f, 0.01597343012690544f, 0.015069978311657906f, 0.016192657873034477f, 0.017473384737968445f, 0.017798839136958122f, 0.012951248325407505f, 0.012373884208500385f, 0.011830097995698452f, 0.016567394137382507f, 0.014703902415931225f, 0.01109615620225668f, 0.013878189958631992f, 0.016271820291876793f, 0.011848719790577888f, 0.014723059721291065f, 0.015665527433156967f, 0.010436942800879478f, 0.012786147184669971f, 0.026921920478343964f, 0.016373159363865852f);
static const ai_layer_format_type conv2d_26_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;



static const ai_i8 gemm_29_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 gemm_29_t_out_0_fmt_zero_const_s8 = 16;
static const ai_u16 gemm_29_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 gemm_29_t_out_0_shape_ch_const_u16 = 1;
static const ai_u32 gemm_29_t_out_0_shape_h_w_prod_const_u32 = 1;
static const ai_float gemm_29_t_in_0_fmt_scale_const_f32 = 0.030651606619358063f;
static const ai_float gemm_29_t_out_0_fmt_scale_const_f32 = 0.05076202005147934f;
static const ai_float gemm_29_t_weight_0_fmt_scale_const_f32 = 0.001935344422236085f;

STAI_API_ENTRY
stai_return_code stai_network_run(
  stai_network* network,
  const stai_run_mode mode)
{
   STAI_UNUSED(mode)
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_ACTIVATIONS) != STAI_FLAG_ACTIVATIONS,
        STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_INPUTS) != STAI_FLAG_INPUTS,
                  STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_OUTPUTS) != STAI_FLAG_OUTPUTS,
                  STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_WEIGHTS) != STAI_FLAG_WEIGHTS,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)


  /* LITE_KERNEL_SECTION BEGIN conv2d_0 */
  {
      const ai_i8* conv2d_0_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_inputs[0] + 0);
    const ai_i8* conv2d_0_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 4);
    const ai_i32* conv2d_0_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 436);
    ai_i8* conv2d_0_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 19840);
    ai_i16* conv2d_0_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 220544);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(0, 1, {(stai_ptr) conv2d_0_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_rgb_sssa8_ch(conv2d_0_t_in_0_ptr_const_s8, conv2d_0_t_in_0_shape_w_const_u16, conv2d_0_t_weight_0_ptr_const_s8, conv2d_0_t_out_0_shape_ch_const_u16, conv2d_0_t_weight_0_shape_w_const_u16, conv2d_0_l_pad_W_0_const_s32, conv2d_0_l_stride_0_const_u16, conv2d_0_t_weight_1_ptr_const_s32, conv2d_0_t_in_0_fmt_zero_const_s8, conv2d_0_t_out_0_fmt_zero_const_s8, conv2d_0_t_in_0_fmt_scale_const_f32, conv2d_0_t_out_0_fmt_scale_const_f32, conv2d_0_t_weight_0_fmt_scale_const_f32, conv2d_0_l_out_ch_format_const_layer_format_type, conv2d_0_t_out_0_ptr_s8, conv2d_0_t_out_0_shape_w_const_u16, 1196, conv2d_0_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(0, 1, {(stai_ptr) conv2d_0_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_0 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_1_pad_before */
  {
      const ai_ptr conv2d_1_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 19840);
    ai_ptr conv2d_1_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 12608);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_1_pad_before_t_in_0_ptr_const_ptr, conv2d_1_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_1_pad_before_v_pad_constant_value_const_s8), conv2d_1_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_1_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1792), (ai_i32)(1824), (ai_i32)(1824), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_1_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_1 */
  {
      const ai_i8* conv2d_1_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 12608);
    const ai_i8* conv2d_1_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 500);
    const ai_i32* conv2d_1_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 644);
    ai_i8* conv2d_1_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 220544);
    ai_i16* conv2d_1_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 12012);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_1_t_in_0_ptr_const_s8, conv2d_1_t_in_0_shape_w_const_u16, conv2d_1_t_in_0_shape_h_const_u16, conv2d_1_t_in_0_shape_ch_const_u16, conv2d_1_t_weight_0_ptr_const_s8, conv2d_1_l_stride_1_const_u16, conv2d_1_l_stride_0_const_u16, conv2d_1_t_weight_1_ptr_const_s32, conv2d_1_t_in_0_fmt_zero_const_s8, conv2d_1_t_out_0_fmt_zero_const_s8, conv2d_1_t_in_0_fmt_scale_const_f32, conv2d_1_t_out_0_fmt_scale_const_f32, conv2d_1_t_weight_0_fmt_scale_const_f32, conv2d_1_t_out_0_ptr_s8, conv2d_1_t_out_0_shape_w_const_u16, conv2d_1_t_out_0_shape_h_const_u16, 0, 593, conv2d_1_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_1 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_2 */
  {
      const ai_i8* conv2d_2_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 220544);
    const ai_i8* conv2d_2_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 708);
    const ai_i32* conv2d_2_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 1220);
    ai_i8* conv2d_2_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 16256);
    ai_i16* conv2d_2_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 12012);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, {(stai_ptr) conv2d_2_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_2_t_in_0_ptr_const_s8, conv2d_2_t_in_0_shape_w_const_u16, conv2d_2_t_in_0_shape_h_const_u16, conv2d_2_l_stride_1_const_u16, conv2d_2_l_stride_0_const_u16, conv2d_2_t_in_0_shape_ch_const_u16, conv2d_2_t_weight_0_ptr_const_s8, conv2d_2_t_out_0_shape_ch_const_u16, conv2d_2_t_weight_1_ptr_const_s32, conv2d_2_t_in_0_fmt_zero_const_s8, conv2d_2_t_out_0_fmt_zero_const_s8, conv2d_2_t_in_0_fmt_scale_const_f32, conv2d_2_t_out_0_fmt_scale_const_f32, conv2d_2_t_weight_0_fmt_scale_const_f32, conv2d_2_l_out_ch_format_const_layer_format_type, conv2d_2_t_out_0_ptr_s8, 1, 384, conv2d_2_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, {(stai_ptr) conv2d_2_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_2 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_3_pad_before */
  {
      const ai_ptr conv2d_3_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 16256);
    ai_ptr conv2d_3_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 1792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(3, 1, {(stai_ptr) conv2d_3_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_3_pad_before_t_in_0_ptr_const_ptr, conv2d_3_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_3_pad_before_v_pad_constant_value_const_s8), conv2d_3_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_3_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(3584), (ai_i32)(0), (ai_i32)(7296), (ai_i32)(0), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, {(stai_ptr) conv2d_3_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_3_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_3 */
  {
      const ai_i8* conv2d_3_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1792);
    const ai_i8* conv2d_3_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 1348);
    const ai_i32* conv2d_3_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 1636);
    ai_i8* conv2d_3_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_3_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 420060);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(3, 1, {(stai_ptr) conv2d_3_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_3_t_in_0_ptr_const_s8, conv2d_3_t_in_0_shape_w_const_u16, conv2d_3_t_in_0_shape_h_const_u16, conv2d_3_t_in_0_shape_ch_const_u16, conv2d_3_t_weight_0_ptr_const_s8, conv2d_3_l_stride_1_const_u16, conv2d_3_l_stride_0_const_u16, conv2d_3_t_weight_1_ptr_const_s32, conv2d_3_t_in_0_fmt_zero_const_s8, conv2d_3_t_out_0_fmt_zero_const_s8, conv2d_3_t_in_0_fmt_scale_const_f32, conv2d_3_t_out_0_fmt_scale_const_f32, conv2d_3_t_weight_0_fmt_scale_const_f32, conv2d_3_t_out_0_ptr_s8, conv2d_3_t_out_0_shape_w_const_u16, conv2d_3_t_out_0_shape_h_const_u16, 0, 1185, conv2d_3_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, {(stai_ptr) conv2d_3_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_3 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_4 */
  {
      const ai_i8* conv2d_4_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_4_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 1764);
    const ai_i32* conv2d_4_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 3812);
    ai_i8* conv2d_4_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 101120);
    ai_i16* conv2d_4_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 100352);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(4, 1, {(stai_ptr) conv2d_4_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_4_t_in_0_ptr_const_s8, conv2d_4_t_in_0_shape_w_const_u16, conv2d_4_t_in_0_shape_h_const_u16, conv2d_4_l_stride_1_const_u16, conv2d_4_l_stride_0_const_u16, conv2d_4_t_in_0_shape_ch_const_u16, conv2d_4_t_weight_0_ptr_const_s8, conv2d_4_t_out_0_shape_ch_const_u16, conv2d_4_t_weight_1_ptr_const_s32, conv2d_4_t_in_0_fmt_zero_const_s8, conv2d_4_t_out_0_fmt_zero_const_s8, conv2d_4_t_in_0_fmt_scale_const_f32, conv2d_4_t_out_0_fmt_scale_const_f32, conv2d_4_t_weight_0_fmt_scale_const_f32, conv2d_4_l_out_ch_format_const_layer_format_type, conv2d_4_t_out_0_ptr_s8, 1, 768, conv2d_4_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(4, 1, {(stai_ptr) conv2d_4_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_4 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_5_pad_before */
  {
      const ai_ptr conv2d_5_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 101120);
    ai_ptr conv2d_5_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 86528);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 1, {(stai_ptr) conv2d_5_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_5_pad_before_t_in_0_ptr_const_ptr, conv2d_5_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_5_pad_before_v_pad_constant_value_const_s8), conv2d_5_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_5_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(3584), (ai_i32)(3712), (ai_i32)(3712), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr) conv2d_5_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_5_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_5 */
  {
      const ai_i8* conv2d_5_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 86528);
    const ai_i8* conv2d_5_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 4068);
    const ai_i32* conv2d_5_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 4644);
    ai_i8* conv2d_5_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 82944);
    ai_i16* conv2d_5_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 1, {(stai_ptr) conv2d_5_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_5_t_in_0_ptr_const_s8, conv2d_5_t_in_0_shape_w_const_u16, conv2d_5_t_in_0_shape_h_const_u16, conv2d_5_t_in_0_shape_ch_const_u16, conv2d_5_t_weight_0_ptr_const_s8, conv2d_5_l_stride_1_const_u16, conv2d_5_l_stride_0_const_u16, conv2d_5_t_weight_1_ptr_const_s32, conv2d_5_t_in_0_fmt_zero_const_s8, conv2d_5_t_out_0_fmt_zero_const_s8, conv2d_5_t_in_0_fmt_scale_const_f32, conv2d_5_t_out_0_fmt_scale_const_f32, conv2d_5_t_weight_0_fmt_scale_const_f32, conv2d_5_t_out_0_ptr_s8, conv2d_5_t_out_0_shape_w_const_u16, conv2d_5_t_out_0_shape_h_const_u16, 0, 2369, conv2d_5_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr) conv2d_5_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_5 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_6 */
  {
      const ai_i8* conv2d_6_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 82944);
    const ai_i8* conv2d_6_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 4900);
    const ai_i32* conv2d_6_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 8996);
    ai_i8* conv2d_6_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 79360);
    ai_i16* conv2d_6_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(6, 1, {(stai_ptr) conv2d_6_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_6_t_in_0_ptr_const_s8, conv2d_6_t_in_0_shape_w_const_u16, conv2d_6_t_in_0_shape_h_const_u16, conv2d_6_l_stride_1_const_u16, conv2d_6_l_stride_0_const_u16, conv2d_6_t_in_0_shape_ch_const_u16, conv2d_6_t_weight_0_ptr_const_s8, conv2d_6_t_out_0_shape_ch_const_u16, conv2d_6_t_weight_1_ptr_const_s32, conv2d_6_t_in_0_fmt_zero_const_s8, conv2d_6_t_out_0_fmt_zero_const_s8, conv2d_6_t_in_0_fmt_scale_const_f32, conv2d_6_t_out_0_fmt_scale_const_f32, conv2d_6_t_weight_0_fmt_scale_const_f32, conv2d_6_l_out_ch_format_const_layer_format_type, conv2d_6_t_out_0_ptr_s8, 1, 896, conv2d_6_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, {(stai_ptr) conv2d_6_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_6 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_7_pad_before */
  {
      const ai_ptr conv2d_7_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 79360);
    ai_ptr conv2d_7_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 64768);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr) conv2d_7_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_7_pad_before_t_in_0_ptr_const_ptr, conv2d_7_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_7_pad_before_v_pad_constant_value_const_s8), conv2d_7_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_7_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(3584), (ai_i32)(0), (ai_i32)(7424), (ai_i32)(0), (ai_i32)(128));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr) conv2d_7_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_7_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_7 */
  {
      const ai_i8* conv2d_7_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 64768);
    const ai_i8* conv2d_7_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 9252);
    const ai_i32* conv2d_7_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 9828);
    ai_i8* conv2d_7_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2372);
    ai_i16* conv2d_7_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr) conv2d_7_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_7_t_in_0_ptr_const_s8, conv2d_7_t_in_0_shape_w_const_u16, conv2d_7_t_in_0_shape_h_const_u16, conv2d_7_t_in_0_shape_ch_const_u16, conv2d_7_t_weight_0_ptr_const_s8, conv2d_7_l_stride_1_const_u16, conv2d_7_l_stride_0_const_u16, conv2d_7_t_weight_1_ptr_const_s32, conv2d_7_t_in_0_fmt_zero_const_s8, conv2d_7_t_out_0_fmt_zero_const_s8, conv2d_7_t_in_0_fmt_scale_const_f32, conv2d_7_t_out_0_fmt_scale_const_f32, conv2d_7_t_weight_0_fmt_scale_const_f32, conv2d_7_t_out_0_ptr_s8, conv2d_7_t_out_0_shape_w_const_u16, conv2d_7_t_out_0_shape_h_const_u16, 0, 2369, conv2d_7_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr) conv2d_7_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_7 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_8 */
  {
      const ai_i8* conv2d_8_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2372);
    const ai_i8* conv2d_8_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 10084);
    const ai_i32* conv2d_8_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 18276);
    ai_i8* conv2d_8_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 52548);
    ai_i16* conv2d_8_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(8, 1, {(stai_ptr) conv2d_8_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_8_t_in_0_ptr_const_s8, conv2d_8_t_in_0_shape_w_const_u16, conv2d_8_t_in_0_shape_h_const_u16, conv2d_8_l_stride_1_const_u16, conv2d_8_l_stride_0_const_u16, conv2d_8_t_in_0_shape_ch_const_u16, conv2d_8_t_weight_0_ptr_const_s8, conv2d_8_t_out_0_shape_ch_const_u16, conv2d_8_t_weight_1_ptr_const_s32, conv2d_8_t_in_0_fmt_zero_const_s8, conv2d_8_t_out_0_fmt_zero_const_s8, conv2d_8_t_in_0_fmt_scale_const_f32, conv2d_8_t_out_0_fmt_scale_const_f32, conv2d_8_t_weight_0_fmt_scale_const_f32, conv2d_8_l_out_ch_format_const_layer_format_type, conv2d_8_t_out_0_ptr_s8, 1, 1536, conv2d_8_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(8, 1, {(stai_ptr) conv2d_8_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_8 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_9_pad_before */
  {
      const ai_ptr conv2d_9_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 52548);
    ai_ptr conv2d_9_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 152900);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {(stai_ptr) conv2d_9_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_9_pad_before_t_in_0_ptr_const_ptr, conv2d_9_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_9_pad_before_v_pad_constant_value_const_s8), conv2d_9_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_9_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(3584), (ai_i32)(3840), (ai_i32)(3840), (ai_i32)(128), (ai_i32)(128));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {(stai_ptr) conv2d_9_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_9_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_9 */
  {
      const ai_i8* conv2d_9_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 152900);
    const ai_i8* conv2d_9_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 18788);
    const ai_i32* conv2d_9_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 19940);
    ai_i8* conv2d_9_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 4740);
    ai_i16* conv2d_9_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {(stai_ptr) conv2d_9_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_9_t_in_0_ptr_const_s8, conv2d_9_t_in_0_shape_w_const_u16, conv2d_9_t_in_0_shape_h_const_u16, conv2d_9_t_in_0_shape_ch_const_u16, conv2d_9_t_weight_0_ptr_const_s8, conv2d_9_l_stride_1_const_u16, conv2d_9_l_stride_0_const_u16, conv2d_9_t_weight_1_ptr_const_s32, conv2d_9_t_in_0_fmt_zero_const_s8, conv2d_9_t_out_0_fmt_zero_const_s8, conv2d_9_t_in_0_fmt_scale_const_f32, conv2d_9_t_out_0_fmt_scale_const_f32, conv2d_9_t_weight_0_fmt_scale_const_f32, conv2d_9_t_out_0_ptr_s8, conv2d_9_t_out_0_shape_w_const_u16, conv2d_9_t_out_0_shape_h_const_u16, 0, 4737, conv2d_9_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {(stai_ptr) conv2d_9_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_9 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_10 */
  {
      const ai_i8* conv2d_10_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 4740);
    const ai_i8* conv2d_10_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 20452);
    const ai_i32* conv2d_10_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 36836);
    ai_i8* conv2d_10_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 105092);
    ai_i16* conv2d_10_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(10, 1, {(stai_ptr) conv2d_10_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_10_t_in_0_ptr_const_s8, conv2d_10_t_in_0_shape_w_const_u16, conv2d_10_t_in_0_shape_h_const_u16, conv2d_10_l_stride_1_const_u16, conv2d_10_l_stride_0_const_u16, conv2d_10_t_in_0_shape_ch_const_u16, conv2d_10_t_weight_0_ptr_const_s8, conv2d_10_t_out_0_shape_ch_const_u16, conv2d_10_t_weight_1_ptr_const_s32, conv2d_10_t_in_0_fmt_zero_const_s8, conv2d_10_t_out_0_fmt_zero_const_s8, conv2d_10_t_in_0_fmt_scale_const_f32, conv2d_10_t_out_0_fmt_scale_const_f32, conv2d_10_t_weight_0_fmt_scale_const_f32, conv2d_10_l_out_ch_format_const_layer_format_type, conv2d_10_t_out_0_ptr_s8, 1, 1792, conv2d_10_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(10, 1, {(stai_ptr) conv2d_10_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_10 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11_pad_before */
  {
      const ai_ptr conv2d_11_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 105092);
    ai_ptr conv2d_11_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 205444);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_11_pad_before_t_in_0_ptr_const_ptr, conv2d_11_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_11_pad_before_v_pad_constant_value_const_s8), conv2d_11_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_11_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(3584), (ai_i32)(0), (ai_i32)(7680), (ai_i32)(0), (ai_i32)(256));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_11_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11 */
  {
      const ai_i8* conv2d_11_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 205444);
    const ai_i8* conv2d_11_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 37348);
    const ai_i32* conv2d_11_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 38500);
    ai_i8* conv2d_11_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 4740);
    ai_i16* conv2d_11_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_11_t_in_0_ptr_const_s8, conv2d_11_t_in_0_shape_w_const_u16, conv2d_11_t_in_0_shape_h_const_u16, conv2d_11_t_in_0_shape_ch_const_u16, conv2d_11_t_weight_0_ptr_const_s8, conv2d_11_l_stride_1_const_u16, conv2d_11_l_stride_0_const_u16, conv2d_11_t_weight_1_ptr_const_s32, conv2d_11_t_in_0_fmt_zero_const_s8, conv2d_11_t_out_0_fmt_zero_const_s8, conv2d_11_t_in_0_fmt_scale_const_f32, conv2d_11_t_out_0_fmt_scale_const_f32, conv2d_11_t_weight_0_fmt_scale_const_f32, conv2d_11_t_out_0_ptr_s8, conv2d_11_t_out_0_shape_w_const_u16, conv2d_11_t_out_0_shape_h_const_u16, 0, 4737, conv2d_11_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_11 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_12 */
  {
      const ai_i8* conv2d_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 4740);
    const ai_i8* conv2d_12_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 39012);
    const ai_i32* conv2d_12_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 71780);
    ai_i8* conv2d_12_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 29828);
    ai_i16* conv2d_12_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(12, 1, {(stai_ptr) conv2d_12_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_12_t_in_0_ptr_const_s8, conv2d_12_t_in_0_shape_w_const_u16, conv2d_12_t_in_0_shape_h_const_u16, conv2d_12_l_stride_1_const_u16, conv2d_12_l_stride_0_const_u16, conv2d_12_t_in_0_shape_ch_const_u16, conv2d_12_t_weight_0_ptr_const_s8, conv2d_12_t_out_0_shape_ch_const_u16, conv2d_12_t_weight_1_ptr_const_s32, conv2d_12_t_in_0_fmt_zero_const_s8, conv2d_12_t_out_0_fmt_zero_const_s8, conv2d_12_t_in_0_fmt_scale_const_f32, conv2d_12_t_out_0_fmt_scale_const_f32, conv2d_12_t_weight_0_fmt_scale_const_f32, conv2d_12_l_out_ch_format_const_layer_format_type, conv2d_12_t_out_0_ptr_s8, 1, 3072, conv2d_12_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(12, 1, {(stai_ptr) conv2d_12_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_12 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_13_pad_before */
  {
      const ai_ptr conv2d_13_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 29828);
    ai_ptr conv2d_13_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 80004);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 1, {(stai_ptr) conv2d_13_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_13_pad_before_t_in_0_ptr_const_ptr, conv2d_13_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_13_pad_before_v_pad_constant_value_const_s8), conv2d_13_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_13_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(3584), (ai_i32)(4096), (ai_i32)(4096), (ai_i32)(256), (ai_i32)(256));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, {(stai_ptr) conv2d_13_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_13_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_13 */
  {
      const ai_i8* conv2d_13_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 80004);
    const ai_i8* conv2d_13_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 72804);
    const ai_i32* conv2d_13_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 75108);
    ai_i8* conv2d_13_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    ai_i16* conv2d_13_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 1, {(stai_ptr) conv2d_13_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_13_t_in_0_ptr_const_s8, conv2d_13_t_in_0_shape_w_const_u16, conv2d_13_t_in_0_shape_h_const_u16, conv2d_13_t_in_0_shape_ch_const_u16, conv2d_13_t_weight_0_ptr_const_s8, conv2d_13_l_stride_1_const_u16, conv2d_13_l_stride_0_const_u16, conv2d_13_t_weight_1_ptr_const_s32, conv2d_13_t_in_0_fmt_zero_const_s8, conv2d_13_t_out_0_fmt_zero_const_s8, conv2d_13_t_in_0_fmt_scale_const_f32, conv2d_13_t_out_0_fmt_scale_const_f32, conv2d_13_t_weight_0_fmt_scale_const_f32, conv2d_13_t_out_0_ptr_s8, conv2d_13_t_out_0_shape_w_const_u16, conv2d_13_t_out_0_shape_h_const_u16, 0, 9473, conv2d_13_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, {(stai_ptr) conv2d_13_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_13 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_14 */
  {
      const ai_i8* conv2d_14_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    const ai_i8* conv2d_14_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 76132);
    const ai_i32* conv2d_14_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 141668);
    ai_i8* conv2d_14_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 59652);
    ai_i16* conv2d_14_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 1, {(stai_ptr) conv2d_14_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_14_t_in_0_ptr_const_s8, conv2d_14_t_in_0_shape_w_const_u16, conv2d_14_t_in_0_shape_h_const_u16, conv2d_14_l_stride_1_const_u16, conv2d_14_l_stride_0_const_u16, conv2d_14_t_in_0_shape_ch_const_u16, conv2d_14_t_weight_0_ptr_const_s8, conv2d_14_t_out_0_shape_ch_const_u16, conv2d_14_t_weight_1_ptr_const_s32, conv2d_14_t_in_0_fmt_zero_const_s8, conv2d_14_t_out_0_fmt_zero_const_s8, conv2d_14_t_in_0_fmt_scale_const_f32, conv2d_14_t_out_0_fmt_scale_const_f32, conv2d_14_t_weight_0_fmt_scale_const_f32, conv2d_14_l_out_ch_format_const_layer_format_type, conv2d_14_t_out_0_ptr_s8, 1, 3584, conv2d_14_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, {(stai_ptr) conv2d_14_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_14 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_15_pad_before */
  {
      const ai_ptr conv2d_15_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 59652);
    ai_ptr conv2d_15_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 109828);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(15, 1, {(stai_ptr) conv2d_15_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_15_pad_before_t_in_0_ptr_const_ptr, conv2d_15_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_15_pad_before_v_pad_constant_value_const_s8), conv2d_15_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_15_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(3584), (ai_i32)(4096), (ai_i32)(4096), (ai_i32)(256), (ai_i32)(256));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(15, 1, {(stai_ptr) conv2d_15_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_15_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_15 */
  {
      const ai_i8* conv2d_15_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 109828);
    const ai_i8* conv2d_15_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 142692);
    const ai_i32* conv2d_15_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 144996);
    ai_i8* conv2d_15_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    ai_i16* conv2d_15_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(15, 1, {(stai_ptr) conv2d_15_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_15_t_in_0_ptr_const_s8, conv2d_15_t_in_0_shape_w_const_u16, conv2d_15_t_in_0_shape_h_const_u16, conv2d_15_t_in_0_shape_ch_const_u16, conv2d_15_t_weight_0_ptr_const_s8, conv2d_15_l_stride_1_const_u16, conv2d_15_l_stride_0_const_u16, conv2d_15_t_weight_1_ptr_const_s32, conv2d_15_t_in_0_fmt_zero_const_s8, conv2d_15_t_out_0_fmt_zero_const_s8, conv2d_15_t_in_0_fmt_scale_const_f32, conv2d_15_t_out_0_fmt_scale_const_f32, conv2d_15_t_weight_0_fmt_scale_const_f32, conv2d_15_t_out_0_ptr_s8, conv2d_15_t_out_0_shape_w_const_u16, conv2d_15_t_out_0_shape_h_const_u16, 0, 9473, conv2d_15_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(15, 1, {(stai_ptr) conv2d_15_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_15 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_16 */
  {
      const ai_i8* conv2d_16_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    const ai_i8* conv2d_16_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 146020);
    const ai_i32* conv2d_16_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 211556);
    ai_i8* conv2d_16_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 59652);
    ai_i16* conv2d_16_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(16, 1, {(stai_ptr) conv2d_16_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_16_t_in_0_ptr_const_s8, conv2d_16_t_in_0_shape_w_const_u16, conv2d_16_t_in_0_shape_h_const_u16, conv2d_16_l_stride_1_const_u16, conv2d_16_l_stride_0_const_u16, conv2d_16_t_in_0_shape_ch_const_u16, conv2d_16_t_weight_0_ptr_const_s8, conv2d_16_t_out_0_shape_ch_const_u16, conv2d_16_t_weight_1_ptr_const_s32, conv2d_16_t_in_0_fmt_zero_const_s8, conv2d_16_t_out_0_fmt_zero_const_s8, conv2d_16_t_in_0_fmt_scale_const_f32, conv2d_16_t_out_0_fmt_scale_const_f32, conv2d_16_t_weight_0_fmt_scale_const_f32, conv2d_16_l_out_ch_format_const_layer_format_type, conv2d_16_t_out_0_ptr_s8, 1, 3584, conv2d_16_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(16, 1, {(stai_ptr) conv2d_16_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_16 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_17_pad_before */
  {
      const ai_ptr conv2d_17_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 59652);
    ai_ptr conv2d_17_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 109828);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(17, 1, {(stai_ptr) conv2d_17_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_17_pad_before_t_in_0_ptr_const_ptr, conv2d_17_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_17_pad_before_v_pad_constant_value_const_s8), conv2d_17_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_17_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(3584), (ai_i32)(4096), (ai_i32)(4096), (ai_i32)(256), (ai_i32)(256));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(17, 1, {(stai_ptr) conv2d_17_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_17_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_17 */
  {
      const ai_i8* conv2d_17_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 109828);
    const ai_i8* conv2d_17_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 212580);
    const ai_i32* conv2d_17_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 214884);
    ai_i8* conv2d_17_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    ai_i16* conv2d_17_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(17, 1, {(stai_ptr) conv2d_17_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_17_t_in_0_ptr_const_s8, conv2d_17_t_in_0_shape_w_const_u16, conv2d_17_t_in_0_shape_h_const_u16, conv2d_17_t_in_0_shape_ch_const_u16, conv2d_17_t_weight_0_ptr_const_s8, conv2d_17_l_stride_1_const_u16, conv2d_17_l_stride_0_const_u16, conv2d_17_t_weight_1_ptr_const_s32, conv2d_17_t_in_0_fmt_zero_const_s8, conv2d_17_t_out_0_fmt_zero_const_s8, conv2d_17_t_in_0_fmt_scale_const_f32, conv2d_17_t_out_0_fmt_scale_const_f32, conv2d_17_t_weight_0_fmt_scale_const_f32, conv2d_17_t_out_0_ptr_s8, conv2d_17_t_out_0_shape_w_const_u16, conv2d_17_t_out_0_shape_h_const_u16, 0, 9473, conv2d_17_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(17, 1, {(stai_ptr) conv2d_17_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_17 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_18 */
  {
      const ai_i8* conv2d_18_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    const ai_i8* conv2d_18_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 215908);
    const ai_i32* conv2d_18_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 281444);
    ai_i8* conv2d_18_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 59652);
    ai_i16* conv2d_18_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(18, 1, {(stai_ptr) conv2d_18_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_18_t_in_0_ptr_const_s8, conv2d_18_t_in_0_shape_w_const_u16, conv2d_18_t_in_0_shape_h_const_u16, conv2d_18_l_stride_1_const_u16, conv2d_18_l_stride_0_const_u16, conv2d_18_t_in_0_shape_ch_const_u16, conv2d_18_t_weight_0_ptr_const_s8, conv2d_18_t_out_0_shape_ch_const_u16, conv2d_18_t_weight_1_ptr_const_s32, conv2d_18_t_in_0_fmt_zero_const_s8, conv2d_18_t_out_0_fmt_zero_const_s8, conv2d_18_t_in_0_fmt_scale_const_f32, conv2d_18_t_out_0_fmt_scale_const_f32, conv2d_18_t_weight_0_fmt_scale_const_f32, conv2d_18_l_out_ch_format_const_layer_format_type, conv2d_18_t_out_0_ptr_s8, 1, 3584, conv2d_18_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(18, 1, {(stai_ptr) conv2d_18_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_18 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_19_pad_before */
  {
      const ai_ptr conv2d_19_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 59652);
    ai_ptr conv2d_19_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 109828);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(19, 1, {(stai_ptr) conv2d_19_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_19_pad_before_t_in_0_ptr_const_ptr, conv2d_19_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_19_pad_before_v_pad_constant_value_const_s8), conv2d_19_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_19_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(3584), (ai_i32)(4096), (ai_i32)(4096), (ai_i32)(256), (ai_i32)(256));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(19, 1, {(stai_ptr) conv2d_19_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_19_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_19 */
  {
      const ai_i8* conv2d_19_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 109828);
    const ai_i8* conv2d_19_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 282468);
    const ai_i32* conv2d_19_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 284772);
    ai_i8* conv2d_19_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    ai_i16* conv2d_19_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(19, 1, {(stai_ptr) conv2d_19_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_19_t_in_0_ptr_const_s8, conv2d_19_t_in_0_shape_w_const_u16, conv2d_19_t_in_0_shape_h_const_u16, conv2d_19_t_in_0_shape_ch_const_u16, conv2d_19_t_weight_0_ptr_const_s8, conv2d_19_l_stride_1_const_u16, conv2d_19_l_stride_0_const_u16, conv2d_19_t_weight_1_ptr_const_s32, conv2d_19_t_in_0_fmt_zero_const_s8, conv2d_19_t_out_0_fmt_zero_const_s8, conv2d_19_t_in_0_fmt_scale_const_f32, conv2d_19_t_out_0_fmt_scale_const_f32, conv2d_19_t_weight_0_fmt_scale_const_f32, conv2d_19_t_out_0_ptr_s8, conv2d_19_t_out_0_shape_w_const_u16, conv2d_19_t_out_0_shape_h_const_u16, 0, 9473, conv2d_19_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(19, 1, {(stai_ptr) conv2d_19_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_19 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_20 */
  {
      const ai_i8* conv2d_20_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    const ai_i8* conv2d_20_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 285796);
    const ai_i32* conv2d_20_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 351332);
    ai_i8* conv2d_20_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 59652);
    ai_i16* conv2d_20_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(20, 1, {(stai_ptr) conv2d_20_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_20_t_in_0_ptr_const_s8, conv2d_20_t_in_0_shape_w_const_u16, conv2d_20_t_in_0_shape_h_const_u16, conv2d_20_l_stride_1_const_u16, conv2d_20_l_stride_0_const_u16, conv2d_20_t_in_0_shape_ch_const_u16, conv2d_20_t_weight_0_ptr_const_s8, conv2d_20_t_out_0_shape_ch_const_u16, conv2d_20_t_weight_1_ptr_const_s32, conv2d_20_t_in_0_fmt_zero_const_s8, conv2d_20_t_out_0_fmt_zero_const_s8, conv2d_20_t_in_0_fmt_scale_const_f32, conv2d_20_t_out_0_fmt_scale_const_f32, conv2d_20_t_weight_0_fmt_scale_const_f32, conv2d_20_l_out_ch_format_const_layer_format_type, conv2d_20_t_out_0_ptr_s8, 1, 3584, conv2d_20_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(20, 1, {(stai_ptr) conv2d_20_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_20 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_21_pad_before */
  {
      const ai_ptr conv2d_21_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 59652);
    ai_ptr conv2d_21_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 109828);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(21, 1, {(stai_ptr) conv2d_21_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_21_pad_before_t_in_0_ptr_const_ptr, conv2d_21_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_21_pad_before_v_pad_constant_value_const_s8), conv2d_21_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_21_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(3584), (ai_i32)(4096), (ai_i32)(4096), (ai_i32)(256), (ai_i32)(256));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(21, 1, {(stai_ptr) conv2d_21_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_21_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_21 */
  {
      const ai_i8* conv2d_21_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 109828);
    const ai_i8* conv2d_21_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 352356);
    const ai_i32* conv2d_21_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 354660);
    ai_i8* conv2d_21_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    ai_i16* conv2d_21_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(21, 1, {(stai_ptr) conv2d_21_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_21_t_in_0_ptr_const_s8, conv2d_21_t_in_0_shape_w_const_u16, conv2d_21_t_in_0_shape_h_const_u16, conv2d_21_t_in_0_shape_ch_const_u16, conv2d_21_t_weight_0_ptr_const_s8, conv2d_21_l_stride_1_const_u16, conv2d_21_l_stride_0_const_u16, conv2d_21_t_weight_1_ptr_const_s32, conv2d_21_t_in_0_fmt_zero_const_s8, conv2d_21_t_out_0_fmt_zero_const_s8, conv2d_21_t_in_0_fmt_scale_const_f32, conv2d_21_t_out_0_fmt_scale_const_f32, conv2d_21_t_weight_0_fmt_scale_const_f32, conv2d_21_t_out_0_ptr_s8, conv2d_21_t_out_0_shape_w_const_u16, conv2d_21_t_out_0_shape_h_const_u16, 0, 9473, conv2d_21_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(21, 1, {(stai_ptr) conv2d_21_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_21 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_22 */
  {
      const ai_i8* conv2d_22_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    const ai_i8* conv2d_22_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 355684);
    const ai_i32* conv2d_22_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 421220);
    ai_i8* conv2d_22_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 59652);
    ai_i16* conv2d_22_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(22, 1, {(stai_ptr) conv2d_22_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_22_t_in_0_ptr_const_s8, conv2d_22_t_in_0_shape_w_const_u16, conv2d_22_t_in_0_shape_h_const_u16, conv2d_22_l_stride_1_const_u16, conv2d_22_l_stride_0_const_u16, conv2d_22_t_in_0_shape_ch_const_u16, conv2d_22_t_weight_0_ptr_const_s8, conv2d_22_t_out_0_shape_ch_const_u16, conv2d_22_t_weight_1_ptr_const_s32, conv2d_22_t_in_0_fmt_zero_const_s8, conv2d_22_t_out_0_fmt_zero_const_s8, conv2d_22_t_in_0_fmt_scale_const_f32, conv2d_22_t_out_0_fmt_scale_const_f32, conv2d_22_t_weight_0_fmt_scale_const_f32, conv2d_22_l_out_ch_format_const_layer_format_type, conv2d_22_t_out_0_ptr_s8, 1, 3584, conv2d_22_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(22, 1, {(stai_ptr) conv2d_22_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_22 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_23_pad_before */
  {
      const ai_ptr conv2d_23_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 59652);
    ai_ptr conv2d_23_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 109828);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(23, 1, {(stai_ptr) conv2d_23_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_23_pad_before_t_in_0_ptr_const_ptr, conv2d_23_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_23_pad_before_v_pad_constant_value_const_s8), conv2d_23_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_23_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(3584), (ai_i32)(0), (ai_i32)(8192), (ai_i32)(0), (ai_i32)(512));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(23, 1, {(stai_ptr) conv2d_23_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_23_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_23 */
  {
      const ai_i8* conv2d_23_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 109828);
    const ai_i8* conv2d_23_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 422244);
    const ai_i32* conv2d_23_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 424548);
    ai_i8* conv2d_23_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    ai_i16* conv2d_23_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(23, 1, {(stai_ptr) conv2d_23_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_23_t_in_0_ptr_const_s8, conv2d_23_t_in_0_shape_w_const_u16, conv2d_23_t_in_0_shape_h_const_u16, conv2d_23_t_in_0_shape_ch_const_u16, conv2d_23_t_weight_0_ptr_const_s8, conv2d_23_l_stride_1_const_u16, conv2d_23_l_stride_0_const_u16, conv2d_23_t_weight_1_ptr_const_s32, conv2d_23_t_in_0_fmt_zero_const_s8, conv2d_23_t_out_0_fmt_zero_const_s8, conv2d_23_t_in_0_fmt_scale_const_f32, conv2d_23_t_out_0_fmt_scale_const_f32, conv2d_23_t_weight_0_fmt_scale_const_f32, conv2d_23_t_out_0_ptr_s8, conv2d_23_t_out_0_shape_w_const_u16, conv2d_23_t_out_0_shape_h_const_u16, 0, 9473, conv2d_23_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(23, 1, {(stai_ptr) conv2d_23_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_23 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_24 */
  {
      const ai_i8* conv2d_24_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    const ai_i8* conv2d_24_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 425572);
    const ai_i32* conv2d_24_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 556644);
    ai_i8* conv2d_24_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 22020);
    ai_i16* conv2d_24_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(24, 1, {(stai_ptr) conv2d_24_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_24_t_in_0_ptr_const_s8, conv2d_24_t_in_0_shape_w_const_u16, conv2d_24_t_in_0_shape_h_const_u16, conv2d_24_l_stride_1_const_u16, conv2d_24_l_stride_0_const_u16, conv2d_24_t_in_0_shape_ch_const_u16, conv2d_24_t_weight_0_ptr_const_s8, conv2d_24_t_out_0_shape_ch_const_u16, conv2d_24_t_weight_1_ptr_const_s32, conv2d_24_t_in_0_fmt_zero_const_s8, conv2d_24_t_out_0_fmt_zero_const_s8, conv2d_24_t_in_0_fmt_scale_const_f32, conv2d_24_t_out_0_fmt_scale_const_f32, conv2d_24_t_weight_0_fmt_scale_const_f32, conv2d_24_l_out_ch_format_const_layer_format_type, conv2d_24_t_out_0_ptr_s8, 1, 6144, conv2d_24_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(24, 1, {(stai_ptr) conv2d_24_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_24 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_25_pad_before */
  {
      const ai_ptr conv2d_25_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 22020);
    ai_ptr conv2d_25_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 47108);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(25, 1, {(stai_ptr) conv2d_25_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_25_pad_before_t_in_0_ptr_const_ptr, conv2d_25_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_25_pad_before_v_pad_constant_value_const_s8), conv2d_25_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_25_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(3584), (ai_i32)(4608), (ai_i32)(4608), (ai_i32)(512), (ai_i32)(512));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(25, 1, {(stai_ptr) conv2d_25_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_25_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_25 */
  {
      const ai_i8* conv2d_25_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 47108);
    const ai_i8* conv2d_25_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 558692);
    const ai_i32* conv2d_25_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 563300);
    ai_i8* conv2d_25_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 18948);
    ai_i16* conv2d_25_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(25, 1, {(stai_ptr) conv2d_25_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_25_t_in_0_ptr_const_s8, conv2d_25_t_in_0_shape_w_const_u16, conv2d_25_t_in_0_shape_h_const_u16, conv2d_25_t_in_0_shape_ch_const_u16, conv2d_25_t_weight_0_ptr_const_s8, conv2d_25_l_stride_1_const_u16, conv2d_25_l_stride_0_const_u16, conv2d_25_t_weight_1_ptr_const_s32, conv2d_25_t_in_0_fmt_zero_const_s8, conv2d_25_t_out_0_fmt_zero_const_s8, conv2d_25_t_in_0_fmt_scale_const_f32, conv2d_25_t_out_0_fmt_scale_const_f32, conv2d_25_t_weight_0_fmt_scale_const_f32, conv2d_25_t_out_0_ptr_s8, conv2d_25_t_out_0_shape_w_const_u16, conv2d_25_t_out_0_shape_h_const_u16, 0, 18945, conv2d_25_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(25, 1, {(stai_ptr) conv2d_25_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_25 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_26 */
  {
      const ai_i8* conv2d_26_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 18948);
    const ai_i8* conv2d_26_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 565348);
    const ai_i32* conv2d_26_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 827492);
    ai_i8* conv2d_26_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 44036);
    ai_i16* conv2d_26_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(26, 1, {(stai_ptr) conv2d_26_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_26_t_in_0_ptr_const_s8, conv2d_26_t_in_0_shape_w_const_u16, conv2d_26_t_in_0_shape_h_const_u16, conv2d_26_l_stride_1_const_u16, conv2d_26_l_stride_0_const_u16, conv2d_26_t_in_0_shape_ch_const_u16, conv2d_26_t_weight_0_ptr_const_s8, conv2d_26_t_out_0_shape_ch_const_u16, conv2d_26_t_weight_1_ptr_const_s32, conv2d_26_t_in_0_fmt_zero_const_s8, conv2d_26_t_out_0_fmt_zero_const_s8, conv2d_26_t_in_0_fmt_scale_const_f32, conv2d_26_t_out_0_fmt_scale_const_f32, conv2d_26_t_weight_0_fmt_scale_const_f32, conv2d_26_l_out_ch_format_const_layer_format_type, conv2d_26_t_out_0_ptr_s8, 1, 7168, conv2d_26_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(26, 1, {(stai_ptr) conv2d_26_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_26 */
  /* LITE_KERNEL_SECTION BEGIN pool_27 */
  {
    
  forward_lite_ap_integer_INT8_pool_27(net_ctx);
  }
  /* LITE_KERNEL_SECTION END pool_27 */
  /* LITE_KERNEL_SECTION BEGIN gemm_28 */
  {
    
  forward_lite_dense_integer_SSSA_ch_gemm_28(net_ctx);
  }
  /* LITE_KERNEL_SECTION END gemm_28 */
  /* LITE_KERNEL_SECTION BEGIN gemm_29 */
  {
      ai_i8* gemm_29_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 256);
    const ai_i8* gemm_29_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2816);
    const ai_i8* gemm_29_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 895588);
    const ai_i32* gemm_29_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 895716);
    ai_i16* gemm_29_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(29, 1, {(stai_ptr) gemm_29_t_in_0_ptr_const_s8});
    
  forward_lite_dense_is8os8ws8(gemm_29_t_out_0_ptr_s8, gemm_29_t_in_0_ptr_const_s8, gemm_29_t_weight_0_ptr_const_s8, gemm_29_t_weight_1_ptr_const_s32, gemm_29_t_in_0_fmt_zero_const_s8, gemm_29_t_out_0_fmt_zero_const_s8, gemm_29_t_in_0_shape_ch_const_u16, gemm_29_t_out_0_shape_ch_const_u16, gemm_29_t_out_0_shape_h_w_prod_const_u32, gemm_29_t_in_0_fmt_scale_const_f32, gemm_29_t_out_0_fmt_scale_const_f32, gemm_29_t_weight_0_fmt_scale_const_f32, gemm_29_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(29, 1, {(stai_ptr) gemm_29_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END gemm_29 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_30 */
  {
    
  forward_lite_eltwise_integer_INT8_eltwise_30(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_30 */
  return net_ctx->_return_code;
}

/*****************************************************************************/
/*  Getters APIs Section  */
STAI_API_ENTRY
stai_size stai_network_get_context_size()
{
  return (stai_size)STAI_NETWORK_CONTEXT_SIZE;
}

#if defined(HAVE_NETWORK_INFO)
STAI_API_ENTRY
stai_return_code stai_network_get_info(
  stai_network* network,
  stai_network_info* info)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, info==NULL, STAI_ERROR_NETWORK_INVALID_INFO, net_ctx->_return_code)

  // Copy of network info struct
  *info = g_network_info;

  return STAI_SUCCESS;
}
#endif


STAI_API_ENTRY
stai_return_code stai_network_get_activations(
  stai_network* network, stai_ptr* activations, stai_size* n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, !n_activations, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_activations = STAI_NETWORK_ACTIVATIONS_NUM;
for (stai_size idx=0; activations && (idx<STAI_NETWORK_ACTIVATIONS_NUM); idx++) {
    // get address of the activations buffers
    activations[idx] = net_ctx->_activations[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_weights(
  stai_network* network, stai_ptr* weights, stai_size* n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_weights, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_weights = STAI_NETWORK_WEIGHTS_NUM;
for (stai_size idx=0; weights && (idx<STAI_NETWORK_WEIGHTS_NUM); idx++) {
    // get address of the weights buffers
    weights[idx] = net_ctx->_weights[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_inputs(
  stai_network* network, stai_ptr* inputs, stai_size* n_inputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_inputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_inputs = STAI_NETWORK_IN_NUM;
  for (stai_size idx=0; inputs && (idx<STAI_NETWORK_IN_NUM); idx++) {
    inputs[idx] = net_ctx->_inputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_outputs(
  stai_network* network, stai_ptr* outputs, stai_size* n_outputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_outputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_outputs = STAI_NETWORK_OUT_NUM;
  for (stai_size idx=0; outputs && (idx<STAI_NETWORK_OUT_NUM); idx++) {
    outputs[idx] = net_ctx->_outputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_error(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /* return 1st generated error or STAI_SUCCESS if no errors so far */
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_states(
  stai_network* network, stai_ptr* states, stai_size* n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_states, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  /* get the number of internals states (supporting multi-heap also for internal states) */
  *n_states = STAI_NETWORK_STATES_NUM;

  STAI_UNUSED(states)
return net_ctx->_return_code;
}


/*****************************************************************************/
/*  Setters APIs Section  */

STAI_API_ENTRY
stai_return_code stai_network_set_activations(
  stai_network* network,
  const stai_ptr* activations,
  const stai_size n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _activations_alignment[] = STAI_NETWORK_ACTIVATIONS_ALIGNMENTS;
  STAI_PRINT("  [stai_network_set_activations] network(%p) activations[%d]: %p\n\n", net_ctx, n_activations, activations)
  _STAI_SET_ERROR(net_ctx, !activations,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_activations!=STAI_NETWORK_ACTIVATIONS_NUM,
                  STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_NUM, net_ctx->_return_code)

  for (stai_size idx=0; activations && idx<STAI_NETWORK_ACTIVATIONS_NUM; idx++) {
    STAI_PRINT("  activation[%d]: %p\n", idx, activations[idx])
    _STAI_SET_ERROR(net_ctx, activations[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)activations[idx]) & (_activations_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_activations[idx] = activations[idx];
  }
  net_ctx->_inputs[0] = activations[0] + 221740;

  net_ctx->_outputs[0] = activations[0] + 0;
_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_weights(
  stai_network* network,
  const stai_ptr* weights,
  const stai_size n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _weights_alignment[] = STAI_NETWORK_WEIGHTS_ALIGNMENTS;
  _STAI_SET_ERROR(net_ctx, !weights,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_weights!=STAI_NETWORK_WEIGHTS_NUM,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_NUM, net_ctx->_return_code)
  for (stai_size idx=0; weights && idx<STAI_NETWORK_WEIGHTS_NUM; idx++) {
    STAI_PRINT("  weight[%d]: %p\n", idx, weights[idx])
    _STAI_SET_ERROR(net_ctx, weights[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)weights[idx]) & (_weights_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_weights[idx] = weights[idx];
  }_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_inputs(
  stai_network* network,
  const stai_ptr* inputs,
  const stai_size n_inputs)
{
  const uintptr_t _inputs_alignment[] = STAI_NETWORK_IN_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !inputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_inputs!=STAI_NETWORK_IN_NUM,
                  STAI_ERROR_NETWORK_INVALID_IN_NUM, net_ctx->_return_code)

  for (stai_size idx=0; inputs && idx<STAI_NETWORK_IN_NUM; idx++) {
    STAI_PRINT("  input[%d]: %p\n", idx, inputs[idx])
    _STAI_SET_ERROR(net_ctx, inputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)inputs[idx]) & (_inputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_inputs[idx] = inputs[idx];
  }

  _stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_outputs(
  stai_network* network,
  const stai_ptr* outputs,
  const stai_size n_outputs)
{
  const uintptr_t _outputs_alignment[] = STAI_NETWORK_OUT_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !outputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_outputs!=STAI_NETWORK_OUT_NUM,
                  STAI_ERROR_NETWORK_INVALID_OUT_NUM, net_ctx->_return_code)

  for (stai_size idx=0; outputs && idx<n_outputs; idx++) {
    STAI_PRINT("  output[%d]: %p\n", idx, outputs[idx])
    _STAI_SET_ERROR(net_ctx, outputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)outputs[idx]) & (_outputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_outputs[idx] = outputs[idx];
  }

  _stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_states(
  stai_network* network,
  const stai_ptr* states,
  const stai_size n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  STAI_UNUSED(states)
  STAI_UNUSED(n_states)
_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}

STAI_API_ENTRY
stai_return_code stai_network_set_callback(
  stai_network* network, const stai_event_cb cb, void* cb_cookie)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  STAI_PRINT("  set_callback %p cb %p cookie %p\n", net_ctx, cb, cb_cookie)
  // _STAI_SET_ERROR(net_ctx, cb==NULL, STAI_ERROR_NETWORK_INVALID_CALLBACK, net_ctx->_return_code)
  net_ctx->_callback = cb;
  net_ctx->_callback_cookie = cb_cookie;
  return net_ctx->_return_code;
}

#undef _STAI_SET_ERROR
#undef _STAI_CONTEXT_ALIGNMENT
#undef _STAI_CONTEXT_ACQUIRE
#undef _STAI_NETWORK_EVENT_NODE_START_CB
#undef _STAI_NETWORK_EVENT_NODE_STOP_CB
#undef _STAI_NETWORK_MODEL_SIGNATURE
#undef _STAI_NETWORK_DATETIME
#undef _STAI_NETWORK_COMPILE_DATETIME

