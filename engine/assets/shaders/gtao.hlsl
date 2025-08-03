#include "common.hlsli"

ConstantBuffer<camera_data> camera : register(b0, space1);

struct constant_data
{
    uint hzb;
    uint depth_buffer;
    uint normal_buffer;
    uint ao_buffer;
    uint hilbert_lut;
    uint width;
    uint height;
    uint slice_count;
    uint step_count;
    float radius;
    float falloff;
    uint frame;
};
PushConstant(constant_data, constant);

// Hilbert R2 Noise.
float2 spatio_temporal_noise(uint2 coord)
{
    Texture2D<uint> hilbert_lut = ResourceDescriptorHeap[constant.hilbert_lut];
    uint index = hilbert_lut[coord % 64];
    index += 288 * (constant.frame % 64);

    return float2(frac(0.5 + index * float2(0.75487766624669276005, 0.5698402909980532659114)));
}

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    // https://github.com/GameTechDev/XeGTAO/blob/master/Source/Rendering/Shaders/XeGTAO.hlsli

    if (dtid.x >= constant.width || dtid.y >= constant.height)
    {
        return;
    }

    float2 texcoord = get_compute_texcoord(dtid.xy, constant.width, constant.height);

    SamplerState point_clamp_sampler = get_point_clamp_sampler();
    
    RWTexture2D<float4> ao_buffer = ResourceDescriptorHeap[constant.ao_buffer];

    Texture2D<float> depth_buffer = ResourceDescriptorHeap[constant.depth_buffer];
    float depth = depth_buffer.SampleLevel(point_clamp_sampler, texcoord, 0.0);

    // If the depth value is 0 (reverse depth), it means the pixel has no valid depth information.
    if (depth == 0)
    {
        ao_buffer[dtid.xy] = 1.0;
        return;
    }

    // Get the position of the current pixel in view space.
    float3 position_vs = reconstruct_position(depth, texcoord, camera.matrix_p_inv).xyz;
    float3 view = normalize(-position_vs);

    // Get the normal of the current pixel in view space.
    Texture2D<float2> normal_buffer = ResourceDescriptorHeap[constant.normal_buffer];
    float3 normal_ws = octahedron_to_normal(normal_buffer.SampleLevel(point_clamp_sampler, texcoord, 0.0));
    float3 normal_vs = normalize(mul((float3x3)camera.matrix_v, normal_ws));

    // Noise.
    float2 noise = spatio_temporal_noise(dtid.xy);
    const float noise_slice = noise.x;
    const float noise_sample = noise.y;

    // Calculate the size of the radius in pixel.
    float radius_in_pixels = (1.0 / tan(camera.fov * 0.5)) * constant.height * constant.radius / position_vs.z;

    float falloff = constant.radius * constant.falloff;
    float falloff_mul = -1.0 / (constant.radius - falloff + 0.01);
    float falloff_add = constant.radius / (constant.radius - falloff + 0.01);

    float2 texel_size = 1.0 / float2(constant.width, constant.height);

    float min_s = 1.3 / radius_in_pixels;

    Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];

    float occlusion = 0.0;
    for (float slice = 0; slice < constant.slice_count; ++slice)
    {
        float phi = (slice + noise_slice) * PI / constant.slice_count;
        float phi_cos = cos(phi);
        float phi_sin = sin(phi);
        float2 omega = float2(phi_cos, -phi_sin) * radius_in_pixels;

        // Project the normal onto the slice.
        float3 direction = float3(phi_cos, phi_sin, 0.0);
        float3 ortho_direction = direction - view * dot(view, direction);
        float3 axis = normalize(cross(ortho_direction, view));
        float3 projected_normal = normal_vs - axis * dot(axis, normal_vs);
        float projected_normal_length = length(projected_normal);

        // Calculate the angle between the projected normal and the view direction.
        float n_cos = saturate(dot(projected_normal, view)) / projected_normal_length;
        float n = sign(dot(ortho_direction, projected_normal)) * acos(n_cos);

        const float h1_cos_min = cos(n - PI * 0.5);
        const float h2_cos_min = cos(n + PI * 0.5);

        float h1_cos = h1_cos_min;
        float h2_cos = h2_cos_min;

        for (float step = 0; step < constant.step_count; ++step)
        {
            const float step_base_noise = float(slice + step * constant.step_count) * 0.6180339887498948482;
            float step_noise = frac(noise_sample + step_base_noise);

            float s = (step + step_noise) / constant.step_count;
            s = pow(s, 2); // From XeGTAO.
            s += min_s;

            float2 offset = s * omega;
            float level = log2(length(offset)) - 3.3;
            offset = round(offset) * texel_size;

            // Calculate h1.
            float p1_depth = hzb.SampleLevel(point_clamp_sampler, texcoord - offset, level);
            float3 p1_position_vs = reconstruct_position(p1_depth, texcoord - offset, camera.matrix_p_inv).xyz;
            float3 p1_delta = p1_position_vs - position_vs;
            float p1_delta_length = length(p1_delta);
            float p1_cos = dot(view, p1_delta / p1_delta_length);

            // Calculate h2.
            float p2_depth = hzb.SampleLevel(point_clamp_sampler, texcoord + offset, level);
            float3 p2_position_vs = reconstruct_position(p2_depth, texcoord + offset, camera.matrix_p_inv).xyz;
            float3 p2_delta = p2_position_vs - position_vs;
            float p2_delta_length = length(p2_delta);
            float p2_cos = dot(view, p2_delta / p2_delta_length);

            // Falloff.
            float p1_weight = saturate(p1_delta_length * falloff_mul + falloff_add);
            float p2_weight = saturate(p2_delta_length * falloff_mul + falloff_add);
            p1_cos = lerp(h1_cos_min, p1_cos, p1_weight);
            p2_cos = lerp(h2_cos_min, p2_cos, p2_weight);

            h1_cos = max(h1_cos, p1_cos);
            h2_cos = max(h2_cos, p2_cos);
        }
        float h1 = -acos(h1_cos);
        float h2 = acos(h2_cos);

        // From https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
        float iarc1 = (n_cos + 2.0 * h1 * sin(n) - cos(2.0 * h1 - n)) / 4.0;
        float iarc2 = (n_cos + 2.0 * h2 * sin(n) - cos(2.0 * h2 - n)) / 4.0;
        occlusion += (iarc1 + iarc2) * projected_normal_length;
    }

    ao_buffer[dtid.xy] = occlusion / constant.slice_count;
}