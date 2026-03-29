#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint visible_virtual_page_list;
    uint vsm_virtual_page_table;
    uint vsm_physical_shadow_map_static;
    uint vsm_physical_shadow_map_final;
};
PushConstant(constant_data, constant);

[numthreads(16, 16, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    StructuredBuffer<uint> visible_virtual_page_list = ResourceDescriptorHeap[constant.visible_virtual_page_list];
    uint virtual_page_index = visible_virtual_page_list[dtid.z];

    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

    uint physical_page_index = get_physical_page_index(virtual_page.physical_page_coord);
    uint2 physical_texel = get_physical_page_coord(physical_page_index) * PAGE_RESOLUTION + dtid.xy;

    Texture2D<uint> physical_shadow_map_static = ResourceDescriptorHeap[constant.vsm_physical_shadow_map_static];
    RWTexture2D<uint> physical_shadow_map_final = ResourceDescriptorHeap[constant.vsm_physical_shadow_map_final];

    physical_shadow_map_final[physical_texel] = max(physical_shadow_map_static[physical_texel], physical_shadow_map_final[physical_texel]);
}