#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint lru_buffer;
    uint lru_curr_index;
    uint vsm_physical_texture;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    StructuredBuffer<uint> lru_buffer = ResourceDescriptorHeap[constant.lru_buffer];

    uint physical_page_index = lru_buffer[get_lru_offset(constant.lru_curr_index) + dtid.z];
    uint2 physical_page_coord = get_physical_page_coord(physical_page_index) * PAGE_RESOLUTION + dtid.xy;

    RWTexture2D<uint> physical_texture = ResourceDescriptorHeap[constant.vsm_physical_texture];
    physical_texture[physical_page_coord] = 0;
}