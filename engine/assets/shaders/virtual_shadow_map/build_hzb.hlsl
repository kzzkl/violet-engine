#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint vsm_physical_page_table;
    uint prev_buffer;
    uint next_buffer;
    uint hzb_sampler;
    uint next_size;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    uint physical_page_index = dtid.z;

    RWStructuredBuffer<uint4> physical_page_table = ResourceDescriptorHeap[constant.vsm_physical_page_table];
    vsm_physical_page physical_page = vsm_physical_page::unpack(physical_page_table[physical_page_index]);

    if ((physical_page.flags & PHYSICAL_PAGE_FLAG_HZB_DIRTY) == 0 || dtid.x >= constant.next_size || dtid.y >= constant.next_size)
    {
        return;
    }

    RWTexture2D<float> next_buffer = ResourceDescriptorHeap[constant.next_buffer];

    uint2 next_texel = dtid.xy + get_physical_page_coord(physical_page_index) * constant.next_size;

    float depth;
    if (constant.next_size == PAGE_RESOLUTION / 2)
    {
        Texture2D<uint> physical_shadow_map = ResourceDescriptorHeap[constant.prev_buffer];

        uint2 prev_texel = next_texel * 2;

        float depth0 = asfloat(physical_shadow_map[prev_texel]);
        float depth1 = asfloat(physical_shadow_map[prev_texel + uint2(1, 0)]);
        float depth2 = asfloat(physical_shadow_map[prev_texel + uint2(0, 1)]);
        float depth3 = asfloat(physical_shadow_map[prev_texel + uint2(1, 1)]);

        depth = min(min(depth0, depth1), min(depth2, depth3));
    }
    else
    {
        Texture2D<float> prev_buffer = ResourceDescriptorHeap[constant.prev_buffer];
        SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];

        depth = prev_buffer.SampleLevel(hzb_sampler, float2(next_texel) / float2(PHYSICAL_PAGE_TABLE_SIZE_X, PHYSICAL_PAGE_TABLE_SIZE_Y) * constant.next_size, 0, int2(1, 1));
    }

    next_buffer[next_texel] = depth;

    if (constant.next_size == 1)
    {
        physical_page.flags &= ~PHYSICAL_PAGE_FLAG_HZB_DIRTY;
        physical_page_table[physical_page_index] = physical_page.pack();
    }
}