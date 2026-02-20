#include "common.hlsli"

struct constant_data
{
    uint shadow_mask;
    uint debug_output;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void debug_shadow_mask(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> debug_output = ResourceDescriptorHeap[constant.debug_output];

    uint width;
    uint height;
    debug_output.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    Texture2D<float> shadow_mask = ResourceDescriptorHeap[constant.shadow_mask];
    float shadow = shadow_mask[dtid.xy];
    debug_output[dtid.xy] = float4(shadow, shadow, shadow, 1.0);
}