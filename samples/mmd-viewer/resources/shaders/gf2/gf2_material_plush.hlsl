#include "gf2/gf2_material.hlsli"

struct gf2_material_plush
{
    uint diffuse_texture;
    uint normal_texture;
    uint noise_texture;
    uint ramp_texture;
    uint brdf_lut;
};

fs_output fs_main(vs_output input)
{
    SamplerState linear_repeat_sampler = get_linear_repeat_sampler();
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();
    
    gf2_material_plush material = load_material<gf2_material_plush>(scene.material_buffer, input.material_address);

    Texture2D<float4> diffuse_texture = ResourceDescriptorHeap[material.diffuse_texture];
    Texture2D<float4> normal_texture = ResourceDescriptorHeap[material.normal_texture];

    float3 albedo = diffuse_texture.Sample(linear_repeat_sampler, input.texcoord).rgb;
    float roughness = 1.0;
    float metallic = 0.0;

    float3 V = normalize(camera.position - input.position_ws);
    float3 N = get_normal(input, normal_texture.Sample(linear_repeat_sampler, input.texcoord).xyz);

    float3 direct_lighting = direct_light(N, V, albedo, roughness, metallic, material.ramp_texture);
    float3 indirect_lighting = indirect_light(N, V, albedo, roughness, metallic, material.brdf_lut);

    fs_output output;
    output.albedo = float4(direct_lighting + indirect_lighting, 1.0);
    output.material = 0.0;
    output.normal = normal_to_octahedron(N);
    output.emissive = 0.0;

    return output;
}