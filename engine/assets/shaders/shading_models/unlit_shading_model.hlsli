#include "shading/shading_model.hlsli"

struct unlit_shading_model
{
    float3 albedo;

    static unlit_shading_model create(shading_context context, surface surface, camera_data camera)
    {
        unlit_shading_model shading_model;
        shading_model.albedo = surface.albedo;
        return shading_model;
    }

    float3 evaluate_direct_lighting(shading_context context, light_data light, float shadow_mask)
    {
        return 0.0;
    }

    float3 evaluate_indirect_lighting(shading_context context, float3 irradiance)
    {
        return albedo;
    }
};