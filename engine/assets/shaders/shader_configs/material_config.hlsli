#ifndef MATERIAL_CONFIG_HLSLI
#define MATERIAL_CONFIG_HLSLI 

#ifndef MATERIAL_SHADER
#define MATERIAL_SHADER "materials/pbr_material.hlsli"
#endif

#include MATERIAL_SHADER

#ifndef MATERIAL_NAME
#define MATERIAL_NAME pbr_material
#endif

using material = MATERIAL_NAME;

#include "material.hlsli"

struct material_data
{
    material_common common;
    material data;
};

#endif