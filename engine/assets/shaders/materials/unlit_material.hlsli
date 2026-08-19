#ifndef UNLIT_MATERIAL_HLSLI
#define UNLIT_MATERIAL_HLSLI

#include "material.hlsli"

struct unlit_material
{
    struct varying
    {
        float4 position_cs : SV_POSITION;
        float3 normal_ws : NORMAL;
    };

    float3 albedo;

    float4 evaluate_position_cs(material_context context, mesh mesh, camera_data camera)
    {
        float3 position = mesh.fetch_attribute<float3>(GEOMETRY_ATTRIBUTE_POSITION);
        float4 position_cs = mul(camera.matrix_vp, mul(mesh.get_model_matrix(), float4(position, 1.0)));
        return position_cs;
    }

    varying evaluate_varying(material_context context, mesh mesh, camera_data camera)
    {
        varying varying;
        varying.position_cs = evaluate_position_cs(context, mesh, camera);

        float4x4 matrix_m = mesh.get_model_matrix();

        float3 normal = mesh.fetch_attribute<float3>(GEOMETRY_ATTRIBUTE_NORMAL);
        varying.normal_ws = mul((float3x3)matrix_m, normal);

        return varying;
    }

    surface evaluate_surface(material_context context, varying varying, camera_data camera)
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