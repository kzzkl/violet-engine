#include "color.hlsli"
#include "brdf.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_output
{
    float4 position : SV_POSITION;
    float3 position_ws : POSITION_WS;
    float3 normal_ws : NORMAL_WS;
    float3 tangent_ws : TANGENT_WS;
    float2 texcoord : TEXCOORD;
    float4 texcoord2 : TEXCOORD2;
    uint material_address : MATERIAL_ADDRESS;
};

struct gf2_material_eye
{
    uint blend_texture;
};

float4 fs_main(vs_output input) : SV_TARGET
{
    SamplerState linear_repeat_sampler = get_linear_repeat_sampler();
    
    gf2_material_eye material = load_material<gf2_material_eye>(scene.material_buffer, input.material_address);

    Texture2D<float4> blend_texture = ResourceDescriptorHeap[material.blend_texture];
    return blend_texture.Sample(linear_repeat_sampler, input.texcoord);
}