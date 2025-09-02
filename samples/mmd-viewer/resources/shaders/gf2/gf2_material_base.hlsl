#include "gf2/gf2_material.hlsli"
#include "shading/shading_model.hlsli"

struct gf2_material_base
{
    uint diffuse_texture;
    uint normal_texture;
    uint rmo_texture;
    uint ramp_texture;
    uint brdf_lut;
};

fs_output fs_main(vs_output input)
{
    SamplerState linear_repeat_sampler = get_linear_repeat_sampler();
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();
    
    gf2_material_base material = load_material<gf2_material_base>(scene.material_buffer, input.material_address);

    Texture2D<float4> diffuse_texture = ResourceDescriptorHeap[material.diffuse_texture];
    Texture2D<float4> normal_texture = ResourceDescriptorHeap[material.normal_texture];
    Texture2D<float4> rmo_texture = ResourceDescriptorHeap[material.rmo_texture];

    float3 albedo = diffuse_texture.Sample(linear_repeat_sampler, input.texcoord).rgb;
    float3 rmo = rmo_texture.Sample(linear_repeat_sampler, input.texcoord).rgb;

    float roughness = rmo.r;
    float metallic = rmo.g;
    float ao = rmo.b;

    float3 V = normalize(camera.position - input.position_ws);
    float3 N = get_normal(input, normal_texture.Sample(linear_repeat_sampler, input.texcoord).xyz);

    float3 direct_lighting = direct_light(N, V, albedo, roughness, metallic, material.ramp_texture);
    float3 indirect_lighting = indirect_light(N, V, albedo, roughness, metallic, material.brdf_lut) * ao;

    material_info material_info = load_material_info(scene.material_buffer, input.material_address);

    fs_output output;
    output.albedo = float4(direct_lighting + indirect_lighting, 1.0);
    output.material = 0.0;
    output.normal = pack_gbuffer_normal(N, material_info.shading_model);
    output.emissive = 0.0;

    return output;
}