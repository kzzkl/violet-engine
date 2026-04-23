#include "common.hlsli"

struct constant_data
{
    uint render_target;
    uint exporse;
};
PushConstant(constant_data, constants);

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> render_target = ResourceDescriptorHeap[constants.render_target];

    uint width;
    uint height;
    render_target.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    Texture2D<float> exposure = ResourceDescriptorHeap[constants.exporse];
    float exposure_value = exposure[uint2(0, 0)];

    render_target[dtid.xy].xyz *= exposure_value;
}