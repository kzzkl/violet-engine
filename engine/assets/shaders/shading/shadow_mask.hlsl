#include "virtual_shadow_map/vsm_common.hlsli"
#include "gbuffer.hlsli"

struct constant_data
{
    uint depth_buffer;
    uint normal_buffer;
    uint vsm_buffer;
    uint vsm_virtual_page_table;
    uint vsm_physical_shadow_map;
    uint vsm_directional_buffer;
    uint light_id;
    float normal_offset;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_output
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float3 get_position_offset(float NdotL, float3 normal_ws, float normal_offset)
{
    float texel_size = 2.0 / VIRTUAL_RESOLUTION;
    float offset_scale = saturate(1.0 - NdotL);
    return texel_size * normal_offset * offset_scale * normal_ws;
}

float get_receiver_plane_depth_bias(float3 position_ws, float4x4 cascade0_matrix_vp, uint cascade)
{
    float4 position_ls_cascade0 = mul(cascade0_matrix_vp, float4(position_ws, 1.0));
    position_ls_cascade0 /= position_ls_cascade0.w;
    position_ls_cascade0.xy = position_ls_cascade0.xy * 0.5 + 0.5;

    float3 dx = ddx_fine(position_ls_cascade0.xyz);
    float3 dy = ddy_fine(position_ls_cascade0.xyz);
    dx = ldexp(dx, cascade);
    dy = ldexp(dy, cascade);

    float2 bias_uv;
    bias_uv.x = dy.y * dx.z - dx.y * dy.z;
    bias_uv.y = dx.x * dy.z - dy.x * dx.z;
    bias_uv *= 1.0 / ((dx.x * dy.y) - (dx.y * dy.x));

    float fractional_sampling_error = dot(float2(1.0, 1.0) / VIRTUAL_RESOLUTION, abs(bias_uv));
    return min(fractional_sampling_error, 0.01);
}

float fs_main(vs_output input) : SV_TARGET
{
    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.shadow_casting_light_buffer];
    light_data light = lights[constant.light_id];
    
    float3 position_ws = reconstruct_position(constant.depth_buffer, input.texcoord, camera.matrix_vp_inv).xyz;

    Texture2D<uint> buffer = ResourceDescriptorHeap[constant.normal_buffer];
    uint width;
    uint height;
    buffer.GetDimensions(width, height);
    float3 normal_ws = unpack_gbuffer_normal(constant.normal_buffer, input.texcoord * float2(width, height));

    float distance = length(position_ws - camera.position) * 100.0;

    StructuredBuffer<uint> directional_vsms = ResourceDescriptorHeap[constant.vsm_directional_buffer];

    uint vsm_id = get_directional_vsm_id(directional_vsms, light.vsm_address, camera.camera_id);
    uint cascade = get_directional_cascade(distance);

    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    vsm_data vsm = vsms[vsm_id + cascade];

    position_ws += get_position_offset(dot(normal_ws, light.direction), normal_ws, constant.normal_offset);
    float bias = get_receiver_plane_depth_bias(position_ws, vsms[vsm_id].matrix_vp, cascade);

    float4 position_ls = mul(vsm.matrix_vp, float4(position_ws, 1.0));
    position_ls /= position_ls.w;
    position_ls.xy = position_ls.xy * 0.5 + 0.5;

    float2 virtual_page_coord_f = position_ls.xy * VIRTUAL_PAGE_TABLE_SIZE;
    uint2 virtual_page_coord = floor(virtual_page_coord_f);
    float2 virtual_page_local_uv = frac(virtual_page_coord_f);

    uint virtual_page_index = get_virtual_page_index(vsm_id + cascade, virtual_page_coord);

    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

    Texture2D<uint> physical_shadow_map = ResourceDescriptorHeap[constant.vsm_physical_shadow_map];
    uint2 physical_texel = virtual_page.get_physical_texel(virtual_page_local_uv);
    float depth = asfloat(physical_shadow_map[physical_texel]);

    return depth > position_ls.z + bias ? 0.0 : 1.0;
}