#include "color.hlsli"
#include "brdf.hlsli"
#include "gf2/gf2_material.hlsli"

// float4 fs_main(vs_output input) : SV_TARGET
// {
//     SamplerState linear_repeat_sampler = get_linear_repeat_sampler();
    
//     gf2_material_eye material = load_material<gf2_material_eye>(scene.material_buffer, input.material_address);

//     Texture2D<float4> blend_texture = ResourceDescriptorHeap[material.blend_texture];
//     return blend_texture.Sample(linear_repeat_sampler, input.uv);
// }

struct gf2_material_eye_blend
{
    struct varying
    {
        float4 position_cs : SV_POSITION;
        float2 uv : TEXCOORD;
    };

    uint blend_texture_id;

    varying evaluate_varying(material_context context, mesh mesh)
    {
        varying varying;

        float4x4 matrix_m = mesh.get_model_matrix();

        varying.position_cs = mul(context.camera.matrix_vp, mul(matrix_m, float4(mesh.vertex.position, 1.0)));
        varying.uv = mesh.vertex.uv;

        return varying;
    }

    surface evaluate_surface(material_context context, varying varying)
    {
        SamplerState linear_repeat_sampler = get_linear_repeat_sampler();

        Texture2D<float4> blend_texture = ResourceDescriptorHeap[blend_texture_id];

        surface surface;
        surface.albedo = 0.0;
        surface.roughness = 0.0;
        surface.metallic = 0.0;
        surface.emissive = context.sample_texture(blend_texture, linear_repeat_sampler, varying.uv).rgb;;
        surface.normal_ws = 0.0;

        return surface;
    }
};