#ifndef SHADING_MODEL_HLSLI
#define SHADING_MODEL_HLSLI

#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

static const uint TILE_SIZE = 16;

struct constant_common
{
    uint gbuffers[8];
    uint auxiliary_buffers[4];
    uint render_target;
    uint width;
    uint height;
    uint shading_model;
    uint worklist_buffer;
    uint worklist_offset;

    uint vsm_buffer;
    uint vsm_virtual_page_table;
    uint vsm_physical_texture;
};

bool get_shading_coord(constant_common constant, uint3 gtid, uint3 gid, out uint2 coord)
{
    StructuredBuffer<uint> worklist = ResourceDescriptorHeap[constant.worklist_buffer];

    uint tile_index = worklist[constant.worklist_offset + gid.x];
    coord = uint2(tile_index >> 16, tile_index & 0xFFFF) * TILE_SIZE + gtid.xy;
    return coord.x < constant.width && coord.y < constant.height;
}

float3 unpack_gbuffer_albedo(constant_common constant, uint2 coord)
{
    Texture2D<float4> gbuffer_albbedo = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_ALBEDO]];
    return gbuffer_albbedo[coord].rgb;
}

void unpack_gbuffer_material(constant_common constant, uint2 coord, out float roughness, out float metallic)
{
    Texture2D<float2> gbuffer_material = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_MATERIAL]];
    roughness = max(gbuffer_material[coord].x, 0.03);
    metallic = gbuffer_material[coord].y;
}

uint pack_gbuffer_normal(float3 N, uint shading_model)
{
    float2 oct = normal_to_octahedron(N);
    return (uint(oct.x * 4095.0) << 20) | (uint(oct.y * 4095.0) << 8) | shading_model;
}

bool unpack_gbuffer_normal(constant_common constant, uint2 coord, out float3 N)
{
    Texture2D<uint> gbuffer_normal = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_NORMAL]];
    uint pack = gbuffer_normal[coord];

    uint shading_model = pack & 0xFF;
    if (shading_model != constant.shading_model)
    {
        return false;
    }

    float2 oct = float2(float(pack >> 20) / 4095.0, float((pack & 0x000FFF00) >> 8) / 4095.0);
    N = octahedron_to_normal(oct);

    return true;
}

float3 unpack_gbuffer_normal(uint gbuffer_normal, uint2 coord)
{
    Texture2D<uint> buffer = ResourceDescriptorHeap[gbuffer_normal];
    uint pack = buffer[coord];

    float2 oct = float2(float(pack >> 20) / 4095.0, float((pack & 0x000FFF00) >> 8) / 4095.0);
    return octahedron_to_normal(oct);
}

float3 unpack_gbuffer_emissive(constant_common constant, uint2 coord)
{
    Texture2D<float4> gbuffer_emissive = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_EMISSIVE]];
    return gbuffer_emissive[coord].rgb;
}

struct shadow_parameter
{
    camera_data camera;

    StructuredBuffer<vsm_data> vsms;
    StructuredBuffer<uint> virtual_page_table;
    StructuredBuffer<uint> directional_vsms;
    Texture2D<uint> physical_texture;

    static shadow_parameter create(constant_common constant, scene_data scene, camera_data camera)
    {
        shadow_parameter parameter;
        parameter.camera = camera;
        parameter.directional_vsms = ResourceDescriptorHeap[scene.directional_vsm_buffer];
        parameter.vsms = ResourceDescriptorHeap[constant.vsm_buffer];
        parameter.virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
        parameter.physical_texture = ResourceDescriptorHeap[constant.vsm_physical_texture];
        return parameter;
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
        float2 virtual_page_uv = frac(virtual_page_coord_f);

        uint virtual_page_index = get_virtual_page_index(vsm_id + cascade, virtual_page_coord);
        vsm_virtual_page virtual_page = unpack_virtual_page(virtual_page_table[virtual_page_index]);
        
        uint2 physical_page_coord = virtual_page.get_physical_page_coord(virtual_page_uv);
        float depth = asfloat(physical_texture[physical_page_coord]);

        return depth > position_ls.z ? 0.0 : 1.0;
    }
};

#endif