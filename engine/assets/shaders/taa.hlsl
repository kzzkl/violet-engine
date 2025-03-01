#include "common.hlsli"
#include "color.hlsli"

struct constant_data
{
    uint current_render_target;
    uint history_render_target;
    uint depth_buffer;
    uint motion_vector;
    uint resolved_render_target;
    uint width;
    uint height;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

float2 get_motion_vector(uint2 st)
{
    Texture2D<float> depth_buffer = ResourceDescriptorHeap[constant.depth_buffer];

    float closest_depth = 0.0;
    uint2 closest_offset = 0;

    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            uint2 offset = int2(i, j);
            float depth = depth_buffer[st + offset];
            if (depth > closest_depth)
            {
                closest_depth = depth;
                closest_offset = offset;
            }
        }
    }

    Texture2D<float2> motion_vector_buffer = ResourceDescriptorHeap[constant.motion_vector];
    return motion_vector_buffer[st + closest_offset];
}

float3 clip_color(float3 history_color, uint2 st)
{
    Texture2D<float4> current = ResourceDescriptorHeap[constant.current_render_target];

    history_color = tonemap(history_color);
    history_color = rgb_to_ycocgr(history_color);

    const int2 st_min = int2(0, 0);
    const int2 st_max = int2(constant.width, constant.height) - 1;

    float3 m1 = 0;
    float3 m2 = 0;

    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            float3 color = current[clamp(st + int2(i, j), st_min, st_max)].rgb;
            color = tonemap(color);
            color = rgb_to_ycocgr(color);

            m1 += color;
            m2 += color * color;
        }
    }

    const int N = 9;
    const float VarianceClipGamma = 1.0;
    float3 mu = m1 / N;
    float3 sigma = sqrt(abs(m2 / N - mu * mu));
    float3 aabb_min = mu - VarianceClipGamma * sigma;
    float3 aabb_max = mu + VarianceClipGamma * sigma;

    float3 p_clip = 0.5 * (aabb_max + aabb_min);
    float3 e_clip = 0.5 * (aabb_max - aabb_min);

    float3 v_clip = history_color - p_clip;
    float3 v_unit = v_clip / e_clip;
    float3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    float3 clip_color;
    if (ma_unit > 1.0)
    {
        clip_color = p_clip + v_clip / ma_unit;
    }
    else
    {
        clip_color = history_color;
    }

    clip_color = ycocgr_to_rgb(clip_color);
    clip_color = tonemap_invert(clip_color);

    return clip_color;
}

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= constant.width || dtid.y >= constant.height)
    {
        return;
    }

    float2 texcoord = get_compute_texcoord(dtid.xy, constant.width, constant.height);

#if defined(USE_MOTION_VECTOR)
    float2 motion_vector = get_motion_vector(dtid.xy);
    float2 history_texcoord = texcoord - motion_vector;
#else
    float2 history_texcoord = texcoord;
#endif

    Texture2D<float4> current = ResourceDescriptorHeap[constant.current_render_target];
    RWTexture2D<float4> resolved = ResourceDescriptorHeap[constant.resolved_render_target];

    if (constant.history_render_target != 0 && !any(history_texcoord < 0.0) && !any(history_texcoord > 1.0))
    {
        float3 current_color = current[dtid.xy].rgb;

        Texture2D<float4> history = ResourceDescriptorHeap[constant.history_render_target];
        float3 history_color = history.SampleLevel(get_linear_clamp_sampler(), history_texcoord, 0.0).rgb;
        history_color = clip_color(history_color, dtid.xy);

        resolved[dtid.xy] = float4(lerp(current_color, history_color, 0.9), 1.0);
    }
    else
    {
        resolved[dtid.xy] = current[dtid.xy];
    }
}