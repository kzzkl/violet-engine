#ifndef SHADING_MODEL_CONFIG_HLSLI
#define SHADING_MODEL_CONFIG_HLSLI 

#ifndef SHADING_MODEL_SHADER
#define SHADING_MODEL_SHADER "shading_models/unlit_shading_model.hlsli"
#endif

#include SHADING_MODEL_SHADER

#ifndef SHADING_MODEL_NAME
#define SHADING_MODEL_NAME unlit_shading_model
#endif

using shading_model = SHADING_MODEL_NAME;

#ifdef SHADING_MODEL_HAS_CONSTANT
using shading_model_constant_data = shading_model::constant_data;
#endif

#endif