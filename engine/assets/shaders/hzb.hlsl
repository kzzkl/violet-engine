#include "common.hlsli"

struct constant_data
{
    uint src;
    uint dst_mip0;
    uint dst_mip1;
    uint dst_mip2;
    uint dst_mip3;
    uint hzb_sampler;
};
PushConstant(constant_data, constant);

groupshared float gs_depth[8][8];

#ifdef HZB_REDUCTION_MAX
static const float default_depth = 0.0;
#else
static const float default_depth = 1.0;
#endif

float reduce_depth(float a, float b, float c, float d)
{
#ifdef HZB_REDUCTION_MAX
    return max(max(a, b), max(c, d));
#else
    return min(min(a, b), min(c, d));
#endif
}

[shader("compute")]
[numthreads(8, 8, 1)]
void hzb_reduce(uint3 dtid : SV_DispatchThreadID, uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID)
{
    RWTexture2D<float> dst_mip0 = ResourceDescriptorHeap[constant.dst_mip0];

    uint width;
    uint height;
    dst_mip0.GetDimensions(width, height);

    // Mip 0.
    if (dtid.x < width && dtid.y < height)
    {
        Texture2D<float> src = ResourceDescriptorHeap[constant.src];
        SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];

        float2 uv = get_compute_texcoord(dtid.xy, width, height);

        float depth_mip0 = src.SampleLevel(hzb_sampler, uv, 0.0);
        dst_mip0[dtid.xy] = depth_mip0;
        gs_depth[gtid.y][gtid.x] = depth_mip0;
    }
    else
    {
        gs_depth[gtid.y][gtid.x] = default_depth;
    }

    GroupMemoryBarrierWithGroupSync();

    // Mip 1.
    if (constant.dst_mip1 == 0)
    {
        return;
    }

    if (gtid.x < 4 && gtid.y < 4)
    {
        uint2 base = gtid.xy * 2;

        float depth_mip1 = reduce_depth(
            gs_depth[base.y][base.x],
            gs_depth[base.y][base.x + 1],
            gs_depth[base.y + 1][base.x],
            gs_depth[base.y + 1][base.x + 1]);

        RWTexture2D<float> dst_mip1 = ResourceDescriptorHeap[constant.dst_mip1];

        dst_mip1[gid.xy * 4 + gtid.xy] = depth_mip1;
        gs_depth[base.y][base.x] = depth_mip1;
    }

    GroupMemoryBarrierWithGroupSync();

    // Mip 2.
    if (constant.dst_mip2 == 0)
    {
        return;
    }

    if (gtid.x < 2 && gtid.y < 2)
    {
        uint2 base = gtid.xy * 4;

        float depth_mip2 = reduce_depth(
            gs_depth[base.y][base.x],
            gs_depth[base.y][base.x + 2],
            gs_depth[base.y + 2][base.x],
            gs_depth[base.y + 2][base.x + 2]);

        RWTexture2D<float> dst_mip2 = ResourceDescriptorHeap[constant.dst_mip2];

        dst_mip2[gid.xy * 2 + gtid.xy] = depth_mip2;
        gs_depth[base.y][base.x] = depth_mip2;
    }

    GroupMemoryBarrierWithGroupSync();

    // Mip 3.
    if (constant.dst_mip3 == 0)
    {
        return;
    }

    if (gtid.x == 0 && gtid.y == 0)
    {
        float depth_mip3 = reduce_depth(
            gs_depth[0][0],
            gs_depth[0][4],
            gs_depth[4][0],
            gs_depth[4][4]);

        RWTexture2D<float> dst_mip3 = ResourceDescriptorHeap[constant.dst_mip3];

        dst_mip3[gid.xy] = depth_mip3;
    }
}

[shader("compute")]
[numthreads(16, 16, 1)]
void hzb_copy(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D<float> src = ResourceDescriptorHeap[constant.src];
    RWTexture2D<float> dst = ResourceDescriptorHeap[constant.dst_mip0];

    uint width;
    uint height;
    dst.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    dst[dtid.xy] = src[dtid.xy];
}