#include "common.hlsli"

struct constant_data
{
    uint current_buffer;
    uint history_buffer;
    uint indirect_diffuse;
    uint motion_vector;
    float2 texel_size;
    uint history_valid;
};
PushConstant(constant_data, constant);

void clip_aabb(
    float3 color,
    float aabb_scale,
    float2 uv,
    float2 texel_size,
    Texture2D<float4> buffer,
    SamplerState sampler,
    out float3 aabb_min,
    out float3 aabb_max)
{
    const int2 offset[8] = {
        int2(-1, -1),
        int2(0, -1),
        int2(1, -1),
        int2(-1, 0),
        int2(1, 0),
        int2(-1, 1),
        int2(0, 1),
        int2(1, 1),
    };

    float3 colors[9];

    for (int i = 0; i < 8; ++i)
    {
        colors[i] = buffer.SampleLevel(sampler, uv + offset[i] * texel_size, 0.0).xyz;
    }
    colors[8] = color;

    float3 m1 = 0.0;
    float3 m2 = 0.0;

    for (int i = 0; i < 9; ++i)
    {
        m1 += colors[i];
        m2 += colors[i] * colors[i];
    }

    float3 mean = m1 / 9.0;
    float3 stddev = sqrt((m2 / 9.0) - mean * mean);

    aabb_min = mean - aabb_scale * stddev;
    aabb_max = mean + aabb_scale * stddev;

    aabb_min = min(aabb_min, color);
    aabb_max = max(aabb_max, color);
}

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> indirect_diffuse = ResourceDescriptorHeap[constant.indirect_diffuse];

    uint width;
    uint height;
    indirect_diffuse.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height || !constant.history_valid)
    {
        return;
    }

    float2 texcoord = get_compute_texcoord(dtid.xy, width, height);

    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    Texture2D<float4> current_buffer = ResourceDescriptorHeap[constant.current_buffer];
    Texture2D<float4> history_buffer = ResourceDescriptorHeap[constant.history_buffer];
    Texture2D<float2> motion_vector = ResourceDescriptorHeap[constant.motion_vector];

    float2 velocity = motion_vector.SampleLevel(linear_clamp_sampler, texcoord, 0.0).xy;

    float4 curr_color = current_buffer.SampleLevel(linear_clamp_sampler, texcoord, 0.0);

    float2 history_texcoord = texcoord + velocity;
    if (any(history_texcoord < 0.0) || any(history_texcoord > 1.0))
    {
        indirect_diffuse[dtid.xy] = curr_color;
        return;
    }

    float3 prev_color = history_buffer.SampleLevel(linear_clamp_sampler, history_texcoord, 0.0).rgb;
    prev_color = tonemap(prev_color);

    float3 aabb_min;
    float3 aabb_max;
    clip_aabb(curr_color.rgb, 1.0, texcoord, constant.texel_size, current_buffer, linear_clamp_sampler, aabb_min, aabb_max);

    prev_color = clamp(prev_color, aabb_min, aabb_max);

    float3 blended_color = lerp(curr_color.rgb, prev_color, 0.95);
    blended_color = tonemap_invert(blended_color);

    indirect_diffuse[dtid.xy] = float4(blended_color, curr_color.a);
}