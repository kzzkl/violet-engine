#ifndef UNLIT_MATERIAL_HLSLI
#define UNLIT_MATERIAL_HLSLI

#include "material.hlsli"

struct unlit_material
{
    struct varying
    {
        float4 position_cs : SV_POSITION;
        float3 position_ws : POSITION;
        float3 normal_ws : NORMAL;
    };

    float3 albedo;

    varying evaluate_varying(material_context context, mesh mesh)
    {
        varying varying;

        float4x4 matrix_m = mesh.get_model_matrix();

        float3 position = mesh.fetch_attribute<float3>(GEOMETRY_ATTRIBUTE_POSITION);
        varying.position_ws = mul(matrix_m, float4(position, 1.0)).xyz;
        varying.position_cs = mul(context.camera.matrix_vp, float4(varying.position_ws, 1.0));

        float3 normal = mesh.fetch_attribute<float3>(GEOMETRY_ATTRIBUTE_NORMAL);
        varying.normal_ws = mul((float3x3)matrix_m, normal);

        return varying;
    }

    surface evaluate_surface(material_context context, varying varying)
    {
        surface surface;

        surface.albedo = albedo;
        surface.opacity = 1.0;
        surface.roughness = 1.0;
        surface.metallic = 0.0;
        surface.emissive = 0.0;
        surface.normal_ws = varying.normal_ws;

        return surface;
    }
};

#endif