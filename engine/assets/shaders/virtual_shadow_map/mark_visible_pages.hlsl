#include "common.hlsli"
#include "utils.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint depth_buffer;
    uint visible_light_count;
    uint visible_light_ids;
    uint visible_vsm_ids;
    uint vsm_virtual_page_table;
    uint vsm_buffer;
    uint vsm_directional_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

void mark_directional_vsm_page(float3 position_ws, light_data light, uint vsm_id, uint2 coord)
{
    uint cascade = get_directional_cascade(length(position_ws - camera.position));

    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    vsm_data vsm = vsms[vsm_id + cascade];

    float4 position_ls = mul(vsm.matrix_vp, float4(position_ws, 1.0));
    position_ls /= position_ls.w;

    uint2 virtual_page_coord = floor((position_ls.xy * 0.5 + 0.5) * VIRTUAL_PAGE_TABLE_SIZE);

    RWStructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    uint virtual_page_index = get_virtual_page_index(vsm_id + cascade, virtual_page_coord);
    InterlockedOr(virtual_page_table[virtual_page_index], VIRTUAL_PAGE_FLAG_REQUEST);
}

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D<float> depth_buffer = ResourceDescriptorHeap[constant.depth_buffer];

    uint width;
    uint height;
    depth_buffer.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    float depth = depth_buffer[dtid.xy];
    if (depth == 0.0)
    {
        return;
    }

    StructuredBuffer<uint> visible_light_count = ResourceDescriptorHeap[constant.visible_light_count];
    StructuredBuffer<uint> visible_light_ids = ResourceDescriptorHeap[constant.visible_light_ids];
    StructuredBuffer<uint> directional_vsms = ResourceDescriptorHeap[constant.vsm_directional_buffer];
    
    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.shadow_casting_light_buffer];

    float2 texcoord = get_compute_texcoord(dtid.xy, width, height);
    float4 position_ws = reconstruct_position(depth, texcoord, camera.matrix_vp_inv);

    uint light_count = visible_light_count[0];

    for (uint i = 0; i < light_count; ++i)
    {
        uint light_id = visible_light_ids[i];

        light_data light = lights[light_id];

        if (light.type == LIGHT_DIRECTIONAL)
        {
            uint vsm_id = get_directional_vsm_id(directional_vsms, light.vsm_address, camera.camera_id);
            mark_directional_vsm_page(position_ws.xyz, light, vsm_id, dtid.xy);
        }
    }
}