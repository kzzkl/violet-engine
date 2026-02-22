#include "mesh.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint vsm_buffer;
    uint vsm_virtual_page_table;
    uint vsm_physical_shadow_map;
    uint draw_info_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);

struct vs_output
{
    float4 position_cs : SV_POSITION;
    float3 position_ndc : POSITION_NDC;
    uint vsm_id : VSM_ID;

    float2 texcoord : TEXCOORD;
    uint opacity_mask : OPACITY_MASK;
    uint opacity_cutoff : OPACITY_CUTOFF;
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

    output.texcoord = vertex.texcoord;

    material_info material_info = load_material_info(scene.material_buffer, mesh.get_material_address());
    output.opacity_mask = material_info.opacity_mask;
    output.opacity_cutoff = material_info.opacity_cutoff;

    return output;
}

void fs_main(vs_output input)
{
    Texture2D<float4> opacity_mask = ResourceDescriptorHeap[input.opacity_mask];
    SamplerState point_repeat_sampler = get_point_repeat_sampler();

    float mask = opacity_mask.Sample(point_repeat_sampler, input.texcoord).a;
    clip(mask * 255.0 - input.opacity_cutoff);

    float3 position_ndc = input.position_ndc;

    float2 vsm_uv = position_ndc.xy * 0.5 + 0.5;

    float2 virtual_page_coord_f = vsm_uv * VIRTUAL_PAGE_TABLE_SIZE;
    uint2 virtual_page_coord = floor(virtual_page_coord_f);
    float2 virtual_page_local_uv = frac(virtual_page_coord_f);

    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    uint virtual_page_index = get_virtual_page_index(input.vsm_id, virtual_page_coord);
    vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

    if ((virtual_page.flags & VIRTUAL_PAGE_FLAG_REQUEST) == 0)
    {
        return;
    }

    uint2 physical_texel = virtual_page.get_physical_texel(virtual_page_local_uv);
    RWTexture2D<uint> physical_shadow_map = ResourceDescriptorHeap[constant.vsm_physical_shadow_map];
    InterlockedMax(physical_shadow_map[physical_texel], asuint(position_ndc.z));
}