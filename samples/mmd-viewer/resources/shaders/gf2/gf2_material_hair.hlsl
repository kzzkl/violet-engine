#include "gf2/gf2_material.hlsli"

struct gf2_material_hair
{
    using varying = gf2_varying;

    uint diffuse_texture_id;
    uint specular_texture_id;
    uint ramp_texture_id;
    uint brdf_lut_id;

    varying evaluate_varying(material_context context, mesh mesh)
    {
        return gf2_evaluate_varying(context, mesh);
    }

    surface evaluate_surface(material_context context, varying varying)
    {
        SamplerState linear_repeat_sampler = get_linear_repeat_sampler();

        Texture2D<float4> diffuse_texture = ResourceDescriptorHeap[diffuse_texture_id];

        float3 albedo = context.sample_texture(diffuse_texture, linear_repeat_sampler, varying.uv).rgb;
        float roughness = 1.0;
        float metallic = 0.0;

        float3 V = normalize(context.camera.position - varying.position_ws);
        float3 N = normalize(varying.normal_ws);

        float3 lighting = gf2_evaluate_lighting(context, N, V, albedo, roughness, metallic, ramp_texture_id, varying.position_ws);

        surface surface;
        surface.albedo = 0.0;
        surface.roughness = 0.0;
        surface.metallic = 0.0;
        surface.emissive = lighting;
        surface.normal_ws = N;

        return surface;
    }
};