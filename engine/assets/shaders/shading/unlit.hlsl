#include "common.hlsli"
#include "shading/shading_model.hlsli"

struct constant_data
{
    constant_common common;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void cs_main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    uint2 coord;
    if (!get_shading_coord(constant.common, gtid, gid, coord))
    {
        return;
    }

    float3 N;
    if (!unpack_gbuffer_normal(constant.common, coord, N))
    {
        return;
    }

    float3 albedo = unpack_gbuffer_albedo(constant.common, coord);

    RWTexture2D<float4> render_target = ResourceDescriptorHeap[constant.common.render_target];
    render_target[coord] = float4(albedo, 1.0);
}