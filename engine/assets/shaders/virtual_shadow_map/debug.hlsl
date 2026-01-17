#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint render_target;
    uint visible_vsm_ids;
    uint vsm_virtual_page_table;
    uint vsm_physical_page_table;
    uint vsm_projection_buffer;
    uint vsm_physical_texture;
    uint debug_texture;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> render_target = ResourceDescriptorHeap[constant.render_target];

    if (dtid.y < 256)
    {
        // vsm 0 page table
        StructuredBuffer<uint> visible_vsm_ids = ResourceDescriptorHeap[constant.visible_vsm_ids];

        uint vsm_id = visible_vsm_ids[0];

        RWStructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];

        uint2 page_coord = dtid.xy / (256 / VIRTUAL_PAGE_TABLE_SIZE);
        uint virtual_page_index = get_virtual_page_index(vsm_id, page_coord);
        vsm_virtual_page virtual_page = unpack_virtual_page(virtual_page_table[virtual_page_index]);

        StructuredBuffer<vsm_projection> vsm_projections = ResourceDescriptorHeap[constant.vsm_projection_buffer];
        vsm_projection vsm_projection = vsm_projections[vsm_id];

        float factor = 0.1;
        if (page_coord.x < vsm_projection.aabb.x ||
            page_coord.x > vsm_projection.aabb.z ||
            page_coord.y < vsm_projection.aabb.y ||
            page_coord.y > vsm_projection.aabb.w)
        {
            factor = 1.0;
        }

        if (virtual_page.flags & VIRTUAL_PAGE_FLAG_REQUEST)
        {
            if (virtual_page.flags & VIRTUAL_PAGE_FLAG_UNMAPPED)
            {
                render_target[dtid.xy] = float4(0.0, 0.0, 1.0, 1.0);
            }
            else if (virtual_page.flags & VIRTUAL_PAGE_FLAG_CACHE_VALID)
            {
                render_target[dtid.xy] = float4(0.0, 1.0, 0.0, 1.0);
            }
            else
            {
                render_target[dtid.xy] = float4(1.0, 0.0, 0.0, 1.0);
            }
        }
        else
        {
            render_target[dtid.xy] = float4(1.0, 1.0, 1.0, 1.0);
        }

        render_target[dtid.xy].xyz *= factor;
    }
    else if (dtid.y < 512)
    {
        // phsical page table
        StructuredBuffer<vsm_physical_page> physical_page_table = ResourceDescriptorHeap[constant.vsm_physical_page_table];

        uint2 page_coord = dtid.xy;
        page_coord.y -= 256;
        page_coord /= (256 / PHYSICAL_PAGE_TABLE_SIZE);

        uint physical_page_index = get_physical_page_index(page_coord);
        vsm_physical_page physical_page = physical_page_table[physical_page_index];

        if ((physical_page.flags & PHYSICAL_PAGE_FLAG_RESIDENT) == 0)
        {
            render_target[dtid.xy] = float4(0.0, 0.0, 0.0, 1.0);
            return;
        }

        if (physical_page.vsm_id == 0)
        {
            render_target[dtid.xy] = float4(1.0, 0.0, 0.0, 1.0);
        }
        else if (physical_page.vsm_id == 1)
        {
            render_target[dtid.xy] = float4(0.0, 1.0, 0.0, 1.0);
        }
        else if (physical_page.vsm_id == 2)
        {
            render_target[dtid.xy] = float4(0.0, 0.0, 1.0, 1.0);
        }
        else
        {
            render_target[dtid.xy] = float4(1.0, 1.0, 1.0, 1.0);
        }
    }
    else
    {
        SamplerState sampler = get_point_clamp_sampler();
        Texture2D<float> debug_texture = ResourceDescriptorHeap[constant.debug_texture];

        uint2 coord = dtid.xy;
        coord.y -= 512;
        coord *= 4096 / 256;
        
        // float2 uv = float2(coord) / 512.0;

        // render_target[dtid.xy] = debug_texture.SampleLevel(sampler, uv, 0.0);

        Texture2D<uint> physical_texture = ResourceDescriptorHeap[constant.vsm_physical_texture];
        float depth = asfloat(physical_texture[coord]);
        render_target[dtid.xy] = float4(depth, depth, depth, 1.0);
    }
}