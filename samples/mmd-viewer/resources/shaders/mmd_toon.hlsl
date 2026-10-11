#include "shading/shading_model.hlsli"

struct toon_shading_model
{
    float3 color;
    uint2 coord;

    static toon_shading_model create(shading_context context, surface surface, camera_data camera)
    {
        toon_shading_model shading_model;
        shading_model.color = surface.emissive;
        shading_model.coord = context.coord;
        return shading_model;
    }

    float3 evaluate_direct_lighting(shading_context context, light_data light, float shadow)
    {
        return 0.0;
    }

    float3 evaluate_indirect_lighting(shading_context context, float3 irradiance)
    {
        float3 albedo = color;

        if (context.ao_buffer != 0)
        {
            Texture2D<float> ao_buffer = ResourceDescriptorHeap[context.ao_buffer];
            albedo *= ao_buffer[coord];
        }

        return albedo;
    }
};