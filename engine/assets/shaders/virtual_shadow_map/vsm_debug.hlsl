#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"
#include "utils.hlsli"

struct constant_data
{
    uint debug_info;
    uint debug_info_index;
    uint debug_output;
    uint depth_buffer;
    uint vsm_buffer;
    uint vsm_virtual_page_table;
    uint vsm_directional_buffer;
    uint draw_count_buffer;
    uint light_id;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

static const float3 cascade_colors[16] =
{
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0),
    float3(1.0, 1.0, 0.0),
    float3(1.0, 0.0, 1.0),
    float3(0.0, 1.0, 1.0),
    float3(1.0, 0.5, 0.0),
    float3(0.5, 0.0, 1.0),
    float3(0.0, 0.5, 1.0),
    float3(0.5, 1.0, 0.0),
    float3(1.0, 0.0, 0.5),
    float3(0.0, 1.0, 0.5),
    float3(0.7, 0.7, 0.7),
    float3(0.4, 0.4, 0.4),
    float3(0.6, 0.3, 0.0),
    float3(0.3, 0.6, 0.8),
};

struct debug_data
{
    uint cache_hit;
    uint rendered;
    uint unmapped;
    uint static_drawcall;
    uint dynamic_drawcall;
};

[numthreads(8, 8, 1)]
void debug_info(uint3 dtid : SV_DispatchThreadID)
{
    RWStructuredBuffer<debug_data> debug_infos = ResourceDescriptorHeap[constant.debug_info];

    StructuredBuffer<uint> directional_vsms = ResourceDescriptorHeap[constant.vsm_directional_buffer];
    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.shadow_casting_light_buffer];
    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];

    light_data light = lights[constant.light_id];
    if (light.vsm_address == 0xFFFFFFFF)
    {
        return;
    }

    uint cache_hit = 0;
    uint rendered = 0;
    uint unmapped = 0;

    if (light.type == LIGHT_DIRECTIONAL)
    {
        uint2 virtual_page_coord = dtid.xy;
        uint vsm_id = get_directional_vsm_id(directional_vsms, light.vsm_address, camera.camera_id);

        for (int cascade = 0; cascade < 16; ++cascade)
        {
            vsm_data vsm = vsms[vsm_id + cascade];

            uint virtual_page_index = get_virtual_page_index(vsm_id + cascade, virtual_page_coord);
            vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

            if ((virtual_page.flags & VIRTUAL_PAGE_FLAG_REQUEST) == 0)
            {
                continue;
            }
            
            if (virtual_page.flags & VIRTUAL_PAGE_FLAG_UNMAPPED)
            {
                ++unmapped;
            }
            else
            {
                if (virtual_page.flags & VIRTUAL_PAGE_FLAG_CACHE_VALID)
                {
                    ++cache_hit;
                }
                else
                {
                    ++rendered;
                }
            }
        }

        InterlockedAdd(debug_infos[constant.debug_info_index].cache_hit, cache_hit);
        InterlockedAdd(debug_infos[constant.debug_info_index].rendered, rendered);
        InterlockedAdd(debug_infos[constant.debug_info_index].unmapped, unmapped);
    }

    if (dtid.x == 0 && dtid.y == 0)
    {
        StructuredBuffer<uint> draw_counts = ResourceDescriptorHeap[constant.draw_count_buffer];
        debug_infos[constant.debug_info_index].static_drawcall = draw_counts[0];
        debug_infos[constant.debug_info_index].dynamic_drawcall = draw_counts[1];
    }
}

[numthreads(8, 8, 1)]
void debug_page(uint3 dtid : SV_DispatchThreadID)
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

    StructuredBuffer<uint> directional_vsms = ResourceDescriptorHeap[constant.vsm_directional_buffer];
    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.shadow_casting_light_buffer];
    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    RWTexture2D<float4> debug_output = ResourceDescriptorHeap[constant.debug_output];

    float2 texcoord = get_compute_texcoord(dtid.xy, width, height);
    float4 position_ws = reconstruct_position(depth, texcoord, camera.matrix_vp_inv);

    light_data light = lights[constant.light_id];
    if (light.vsm_address == 0xFFFFFFFF)
    {
        return;
    }

    if (light.type == LIGHT_DIRECTIONAL)
    {
        float distance = length(position_ws.xyz - camera.position) * 100.0;
        uint cascade = get_directional_cascade(distance);
        uint vsm_id = get_directional_vsm_id(directional_vsms, light.vsm_address, camera.camera_id) + cascade;

        vsm_data vsm = vsms[vsm_id];

        float4 position_ls = mul(vsm.matrix_vp, position_ws);
        position_ls /= position_ls.w;

        uint2 virtual_page_coord = floor((position_ls.xy * 0.5 + 0.5) * VIRTUAL_PAGE_TABLE_SIZE);
        int2 global_page_index = virtual_page_coord + vsm.page_coord;

        float3 cascade_color = cascade_colors[cascade];
        float3 page_color = to_color(global_page_index.y * VIRTUAL_PAGE_TABLE_SIZE + global_page_index.x);
        debug_output[dtid.xy] = float4(lerp(cascade_color, page_color, 0.5), 1.0);
    }
}

[numthreads(8, 8, 1)]
void debug_page_cache(uint3 dtid : SV_DispatchThreadID)
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

    StructuredBuffer<uint> directional_vsms = ResourceDescriptorHeap[constant.vsm_directional_buffer];
    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.shadow_casting_light_buffer];
    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    RWTexture2D<float4> debug_output = ResourceDescriptorHeap[constant.debug_output];

    float2 texcoord = get_compute_texcoord(dtid.xy, width, height);
    float4 position_ws = reconstruct_position(depth, texcoord, camera.matrix_vp_inv);

    light_data light = lights[constant.light_id];
    if (light.vsm_address == 0xFFFFFFFF)
    {
        return;
    }

    if (light.type == LIGHT_DIRECTIONAL)
    {
        float distance = length(position_ws.xyz - camera.position) * 100.0;
        uint cascade = get_directional_cascade(distance);
        uint vsm_id = get_directional_vsm_id(directional_vsms, light.vsm_address, camera.camera_id) + cascade;

        vsm_data vsm = vsms[vsm_id];

        float4 position_ls = mul(vsm.matrix_vp, position_ws);
        position_ls /= position_ls.w;

        uint2 virtual_page_coord = floor((position_ls.xy * 0.5 + 0.5) * VIRTUAL_PAGE_TABLE_SIZE);
        uint virtual_page_index = get_virtual_page_index(vsm_id, virtual_page_coord);

        vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);
        if ((virtual_page.flags & VIRTUAL_PAGE_FLAG_REQUEST) != 0 && (virtual_page.flags & VIRTUAL_PAGE_FLAG_CACHE_VALID) == 0)
        {
            debug_output[dtid.xy] = float4(1.0, 0.0, 0.0, 1.0);
        }
        else
        {
            debug_output[dtid.xy] = float4(0.0, 1.0, 0.0, 1.0);
        }
    }
}