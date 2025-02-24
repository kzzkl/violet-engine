#include "gf2/gf2_material.hlsli"

struct gf2_material_eye
{
    uint diffuse_texture;
};

fs_output fs_main(vs_output input)
{
    SamplerState linear_repeat_sampler = get_linear_repeat_sampler();
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();
    
    gf2_material_eye material = load_material<gf2_material_eye>(scene.material_buffer, input.material_address);

    Texture2D<float4> diffuse_texture = ResourceDescriptorHeap[material.diffuse_texture];

    fs_output output;
    output.albedo = diffuse_texture.Sample(linear_repeat_sampler, input.texcoord);
    output.material = 0.0;
    output.normal = 0.0;
    output.emissive = 0.0;

    return output;
}