#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint vsm_physical_page_table;
    uint vsm_physical_shadow_map_static;
    uint vsm_physical_shadow_map_final;
};
PushConstant(constant_data, constant);

[numthreads(16, 16, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    uint physical_page_index = dtid.z;

    StructuredBuffer<uint4> physical_page_table = ResourceDescriptorHeap[constant.vsm_physical_page_table];
    vsm_physical_page physical_page = vsm_physical_page::unpack(physical_page_table[physical_page_index]);

    if ((physical_page.flags & PHYSICAL_PAGE_FLAG_REQUEST) == 0)
    {
        return;
    }

    RWTexture2D<uint> physical_shadow_map_static = ResourceDescriptorHeap[constant.vsm_physical_shadow_map_static];
    RWTexture2D<uint> physical_shadow_map_final = ResourceDescriptorHeap[constant.vsm_physical_shadow_map_final];

    uint2 physical_texel = get_physical_page_coord(physical_page_index) * PAGE_RESOLUTION + dtid.xy;

    if (physical_page.flags & PHYSICAL_PAGE_FLAG_NEED_CLEAR)
    {
        physical_shadow_map_static[physical_texel] = 0;
    }

    physical_shadow_map_final[physical_texel] = 0;
}