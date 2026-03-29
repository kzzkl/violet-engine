#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"
#include "utils.hlsli"

struct constant_data
{
    uint vsm_buffer;
    uint vsm_physical_page_table;
    uint vsm_invalidation_buffer;
    uint vsm_invalidation_count;
};
PushConstant(constant_data, constant);

#define INVALIDATION_BATCH_SIZE 64

groupshared float4 gs_spheres[INVALIDATION_BATCH_SIZE];

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    uint physical_page_index = dtid.x;

    RWStructuredBuffer<uint4> physical_page_table = ResourceDescriptorHeap[constant.vsm_physical_page_table];
    StructuredBuffer<float4> invalidation_buffer = ResourceDescriptorHeap[constant.vsm_invalidation_buffer];
    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];

    vsm_physical_page physical_page;
    vsm_data vsm;
    bool is_resident = false;

    if (physical_page_index < PHYSICAL_PAGE_TABLE_PAGE_COUNT)
    {
        physical_page = vsm_physical_page::unpack(physical_page_table[physical_page_index]);
        is_resident = (physical_page.flags & PHYSICAL_PAGE_FLAG_RESIDENT) != 0;

        if (is_resident)
        {
            vsm = vsms[physical_page.vsm_id];
        }
    }

    bool dirty = false;

    for (uint batch_start = 0; batch_start < constant.vsm_invalidation_count; batch_start += INVALIDATION_BATCH_SIZE)
    {
        uint load_index = batch_start + group_index;
        if (load_index < constant.vsm_invalidation_count)
        {
            gs_spheres[group_index] = invalidation_buffer[load_index];
        }
        GroupMemoryBarrierWithGroupSync();

        if (is_resident && !dirty)
        {
            uint batch_count = min(INVALIDATION_BATCH_SIZE, constant.vsm_invalidation_count - batch_start);
            for (uint i = 0; i < batch_count; ++i)
            {
                float4 sphere_ws = gs_spheres[i];

                float4 sphere_vs = mul(vsm.matrix_v, float4(sphere_ws.xyz, 1.0));
                sphere_vs.w = sphere_ws.w;

                float4 projected_aabb;
                project_shpere_orthographic(sphere_vs, vsm.matrix_p[0][0], vsm.matrix_p[1][1], projected_aabb);

                float4 local_page_bounds = (projected_aabb * 0.5 + 0.5) * VIRTUAL_PAGE_TABLE_SIZE;

                int2 world_page_min = int2(floor(local_page_bounds.xy)) + vsm.page_coord;
                int2 world_page_max = int2(floor(local_page_bounds.zw)) + vsm.page_coord;

                if (physical_page.virtual_page_coord.x >= world_page_min.x &&
                    physical_page.virtual_page_coord.x <= world_page_max.x &&
                    physical_page.virtual_page_coord.y >= world_page_min.y &&
                    physical_page.virtual_page_coord.y <= world_page_max.y)
                {
                    dirty = true;
                    break;
                }
            }
        }

        GroupMemoryBarrierWithGroupSync();
    }

    if (dirty)
    {
        physical_page.flags &= ~(PHYSICAL_PAGE_FLAG_REQUEST | PHYSICAL_PAGE_FLAG_RESIDENT);
        physical_page_table[physical_page_index] = physical_page.pack();
    }
}