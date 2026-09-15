#include "gf2/gf2_material.hlsli"

struct gf2_material_eye
{
    using varying = gf2_varying;

    uint diffuse_texture_id;

    varying evaluate_varying(material_context context, mesh mesh)
    {
        return gf2_evaluate_varying(context, mesh);
    }

    surface evaluate_surface(material_context context, varying varying)
    {
        SamplerState linear_repeat_sampler = get_linear_repeat_sampler();

        Texture2D<float4> diffuse_texture = ResourceDescriptorHeap[diffuse_texture_id];

        surface surface;
        surface.albedo = 0.0;
        surface.roughness = 0.0;
        surface.metallic = 0.0;
        surface.emissive = context.sample_texture(diffuse_texture, linear_repeat_sampler, varying.uv).rgb;;
        surface.normal_ws = 0.0;

        return surface;
    }
};