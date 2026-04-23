#include "hzb_trace.hlsli"
#include "gbuffer.hlsli"
#include "spherical_harmonics.hlsli"

struct constant_data
{
    uint scene_color;
    uint normal_buffer;
    float2 hzb_texel_size;
    uint hzb;
    uint hzb_start_level;
    uint hzb_end_level;
    uint motion_vector;
    float2 stbn_cosine_texel_size;
    uint stbn_cosine;
    uint stbn_cosine_slice;
    uint sample_count;
    uint irradiance_sh;
    uint indirect_diffuse;
    float thickness;
    uint iteration_count;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

void build_basis(float3 N, out float3 T, out float3 B)
{
    float sign = N.z >= 0 ? 1.0 : -1.0;
    float a = -1.0 / (sign + N.z);
    float b = N.x * N.y * a;

    T = float3(1.0 + sign * N.x * N.x * a, sign * b, -sign * N.x);
    B = float3(b, sign + N.y * N.y * a, -N.y);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> indirect_diffuse = ResourceDescriptorHeap[constant.indirect_diffuse];

    uint width;
    uint height;
    indirect_diffuse.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    float2 uv = get_compute_texcoord(dtid.xy, width, height);

    Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];

    float depth = hzb.SampleLevel(get_point_clamp_sampler(), uv, constant.hzb_start_level);
    if (depth == 0.0)
    {
        indirect_diffuse[dtid.xy] = float4(0.0, 0.0, 0.0, -1.0);
        return;
    }

    float3 position_ws = reconstruct_position(depth, uv, camera.matrix_vp_inv).xyz;
    float3 normal_ws = unpack_gbuffer_normal(constant.normal_buffer, dtid.xy * 2); // Half resolution.

    Texture2D<float4> scene_color = ResourceDescriptorHeap[constant.scene_color];
    Texture2DArray<float4> stbn_cosine = ResourceDescriptorHeap[constant.stbn_cosine];

    Texture2D<float2> motion_vector = ResourceDescriptorHeap[constant.motion_vector];

    StructuredBuffer<sh9> irradiance_sh = ResourceDescriptorHeap[constant.irradiance_sh];
    sh9 sh = irradiance_sh[0];

    SamplerState point_repeat_sampler = get_point_repeat_sampler();
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float3 ray_origin_ts = float3(uv, depth);

    float3 color = 0.0;

    for (uint i = 0; i < constant.sample_count; ++i)
    {
        float4 stbn = stbn_cosine.SampleLevel(point_repeat_sampler, float3(dtid.xy * constant.stbn_cosine_texel_size, constant.stbn_cosine_slice), 0.0);
        stbn.xyz = stbn.xyz * 2.0 - 1.0;

        float3 T, B;
        build_basis(normal_ws, T, B);

        float3 sample_direction = stbn.x * T + stbn.y * B + stbn.z * normal_ws;
        sample_direction = normalize(sample_direction);

        float3 ray_end_ws = position_ws + sample_direction;
        float4 ray_end_cs = mul(camera.matrix_vp, float4(ray_end_ws, 1.0));
        ray_end_cs /= ray_end_cs.w;
        ray_end_cs.xy = ray_end_cs.xy * float2(0.5, -0.5) + 0.5;

        float3 ray_direction_ts = normalize(ray_end_cs.xyz - ray_origin_ts.xyz);
        float4 hit = hzb_trace(
            ray_origin_ts,
            ray_direction_ts,
            constant.thickness,
            constant.iteration_count,
            hzb,
            constant.hzb_texel_size,
            constant.hzb_start_level,
            constant.hzb_end_level,
            camera.near);

        float2 velocity = motion_vector.SampleLevel(linear_clamp_sampler, hit.xy, 0.0).xy;

        float3 sky = sh.evaluate(sample_direction);
        float3 scene = scene_color.SampleLevel(linear_clamp_sampler, hit.xy + velocity, 0.0).rgb;
        float3 trace_color = lerp(sky, scene, hit.w);

        color += trace_color;
    }

    color /= constant.sample_count;

    indirect_diffuse[dtid.xy] = float4(color, camera.near / depth);
}