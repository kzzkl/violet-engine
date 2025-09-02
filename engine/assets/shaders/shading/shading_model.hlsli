#include "common.hlsli"

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