#ifndef SHADOW_HLSLI
#define SHADOW_HLSLI

#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct shadow_context
{
    camera_data camera;

    StructuredBuffer<vsm_data> vsms;
    StructuredBuffer<uint> virtual_page_table;
    StructuredBuffer<uint> directional_vsms;
    Texture2D<uint> physical_shadow_map;

    static shadow_context create(scene_data scene, camera_data camera)
    {
        shadow_context context;
        context.camera = camera;
        context.directional_vsms = ResourceDescriptorHeap[scene.vsm_directional_buffer];
        context.vsms = ResourceDescriptorHeap[scene.vsm_buffer];
        context.virtual_page_table = ResourceDescriptorHeap[scene.vsm_virtual_page_table];
        context.physical_shadow_map = ResourceDescriptorHeap[scene.vsm_physical_shadow_map];
        return context;
    }

    float get_shadow(light_data light, float3 position_ws)
    {
        float distance = length(position_ws - camera.position) * 100.0;

        uint vsm_id = get_directional_vsm_id(directional_vsms, light.vsm_address, camera.camera_id);
        uint cascade = get_directional_cascade(distance);

        vsm_data vsm = vsms[vsm_id + cascade];

        float4 position_ls = mul(vsm.matrix_vp, float4(position_ws, 1.0));
        position_ls /= position_ls.w;

        float2 virtual_page_coord_f = (position_ls.xy * 0.5 + 0.5) * VIRTUAL_PAGE_TABLE_SIZE;
        uint2 virtual_page_coord = floor(virtual_page_coord_f);
        float2 virtual_page_local_uv = frac(virtual_page_coord_f);

        uint virtual_page_index = get_virtual_page_index(vsm_id + cascade, virtual_page_coord);
        vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

        uint2 physical_texel = virtual_page.get_physical_texel(virtual_page_local_uv);
        float depth = asfloat(physical_shadow_map[physical_texel]);

        return depth > position_ls.z ? 0.0 : 1.0;
    }
};

#endif