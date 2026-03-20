#include "common.hlsli"
#include "spherical_harmonics.hlsli"

struct constant_data
{
    uint environment_map;
    uint buffer_size;
    uint irradiance_sh_src;
    uint irradiance_sh_dst;
};
PushConstant(constant_data, constant);

static const float3 forward_dir[6] = {
    float3(1.0, 0.0, 0.0),
    float3(-1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, -1.0, 0.0),
    float3(0.0, 0.0, 1.0),
    float3(0.0, 0.0, -1.0),
};

static const float3 up_dir[6] = {
    float3(0.0, 1.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, -1.0),
    float3(0.0, 0.0, 1.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 1.0, 0.0),
};

static const float3 right_dir[6] = {
    float3(0.0, 0.0, -1.0),
    float3(0.0, 0.0, 1.0),
    float3(1.0, 0.0, 0.0),
    float3(1.0, 0.0, 0.0),
    float3(1.0, 0.0, 0.0),
    float3(-1.0, 0.0, 0.0),
};

float3 get_direction(uint3 coord, uint width)
{
    float2 offset = float2(coord.xy) / width * 2.0 - 1.0;
    offset.y = -offset.y;

    return normalize(forward_dir[coord.z] + offset.x * right_dir[coord.z] + offset.y * up_dir[coord.z]);
}

float area_element(float x, float y)
{
    return atan2(x * y, sqrt(x * x + y * y + 1));
}

float texel_solid_angle(float2 uv, int size)
{
    float u = (2.0 * uv.x) - 1.0;
    float v = (2.0 * uv.y) - 1.0;

    float resolution_inv = 1.0 / size;

    float x0 = u - resolution_inv;
    float y0 = v - resolution_inv;
    float x1 = u + resolution_inv;
    float y1 = v + resolution_inv;
    float solid_angle = area_element(x0, y0) - area_element(x0, y1) - area_element(x1, y0) + area_element(x1, y1);
 
    return solid_angle;
}

groupshared sh9 gs_sh[64];

void load_irradiance_sh_from_cube(uint thread_id, uint group_index)
{
    TextureCube<float3> environment_map = ResourceDescriptorHeap[constant.environment_map];

    uint width;
    uint height;
    environment_map.GetDimensions(width, height);

    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    sh9 sh = (sh9)0;

    uint3 coord = uint3(thread_id % width, thread_id / width, 0);
    float weight = texel_solid_angle(get_compute_texcoord(coord.xy, width, height), width);

    for (uint i = 0; i < 6; ++i)
    {
        coord.z = i;

        float3 direction = get_direction(coord, width);

        float3 color = environment_map.SampleLevel(linear_clamp_sampler, direction, 0.0);
        color = min(color, 1000.0);

        sh.project(direction, color * weight);
    }

    gs_sh[group_index] = sh;

    GroupMemoryBarrierWithGroupSync();
}

void load_irradiance_sh_from_buffer(uint thread_id, uint group_index)
{
    StructuredBuffer<sh9> irradiance_sh = ResourceDescriptorHeap[constant.irradiance_sh_src];

    if (thread_id < constant.buffer_size)
    {
        gs_sh[group_index] = irradiance_sh[thread_id];
    }
    else
    {
        gs_sh[group_index] = (sh9)0;
    }

    GroupMemoryBarrierWithGroupSync();
}

void reduce(uint thread_id, uint group_index, uint index)
{
    if ((thread_id & index) == 0)
    {
        gs_sh[group_index].add(gs_sh[group_index + index]);
    }

    GroupMemoryBarrierWithGroupSync();
}

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex, uint3 gid : SV_GroupID)
{
#ifdef IRRADIANCE_SH_STAGE_MAIN
    load_irradiance_sh_from_cube(dtid.x, group_index);
#else
    load_irradiance_sh_from_buffer(dtid.x, group_index);
#endif

    reduce(dtid.x, group_index, 1);
    reduce(dtid.x, group_index, 2);
    reduce(dtid.x, group_index, 4);
    reduce(dtid.x, group_index, 8);
    reduce(dtid.x, group_index, 16);
    reduce(dtid.x, group_index, 32);

    if (group_index == 0)
    {
        RWStructuredBuffer<sh9> irradiance_sh = ResourceDescriptorHeap[constant.irradiance_sh_dst];
        irradiance_sh[gid.x] = gs_sh[0];
    }
}