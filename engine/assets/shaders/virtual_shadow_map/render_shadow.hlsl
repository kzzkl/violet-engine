#include "mesh.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint vsm_buffer;
    uint vsm_virtual_page_table;
    uint vsm_physical_texture;
    uint draw_info_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);

struct vs_output
{
    float4 position_cs : SV_POSITION;
    float3 position_ndc : POSITION_NDC;
    uint vsm_id : VSM_ID;
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint draw_id : SV_InstanceID)
{
    StructuredBuffer<vsm_draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
    uint vsm_id = draw_infos[draw_id].vsm_id;
    uint instance_id = draw_infos[draw_id].instance_id;

    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    vsm_data vsm = vsms[vsm_id];

    mesh mesh = mesh::create(instance_id, scene);
    vertex vertex = mesh.fetch_vertex(vertex_id, vsm.matrix_vp);

    vs_output output;
    output.position_cs = vertex.position_cs;
    output.position_ndc = vertex.position_cs.xyz / vertex.position_cs.w;
    output.vsm_id = vsm_id;

    return output;
}

void fs_main(vs_output input)
{
    // float3 position_ndc = input.position_cs.xyz / input.position_cs.w;
    float3 position_ndc = input.position_ndc;

    float2 vsm_uv = position_ndc.xy * 0.5 + 0.5;

    float2 virtual_page_coord_f = vsm_uv * VIRTUAL_PAGE_TABLE_SIZE;
    uint2 virtual_page_coord = floor(virtual_page_coord_f);
    float2 virtual_page_uv = frac(virtual_page_coord_f);

    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    uint virtual_page_index = get_virtual_page_index(input.vsm_id, virtual_page_coord);
    vsm_virtual_page virtual_page = unpack_virtual_page(virtual_page_table[virtual_page_index]);

    if (virtual_page.flags != VIRTUAL_PAGE_FLAG_REQUEST)
    {
        return;
    }

    uint2 physical_page_coord = virtual_page.get_physical_page_coord(virtual_page_uv);
    RWTexture2D<uint> physical_texture = ResourceDescriptorHeap[constant.vsm_physical_texture];
    InterlockedMax(physical_texture[physical_page_coord], asuint(position_ndc.z));
}