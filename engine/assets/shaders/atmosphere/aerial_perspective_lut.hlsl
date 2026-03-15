#include "atmosphere/atmosphere.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    atmosphere_data atmosphere;
    float3 sun_direction;
    float distance_per_slice;
    float3 sun_irradiance;
    uint sample_count;
    float3 frustum_top_left;
    uint transmittance_lut;
    float3 frustum_top_right;
    uint aerial_perspective_lut;
    float3 frustum_bottom_left;
    uint padding0;
    float3 frustum_bottom_right;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

float4 integrate(atmosphere_data atmosphere, float distance, float3 eye, float3 view, Texture2D<float3> transmittance_lut)
{
    float cos_theta = dot(-view, constant.sun_direction);
    float3 rayleigh_phase = atmosphere.get_rayleigh_phase(cos_theta);
    float mie_phase = atmosphere.get_mie_phase(cos_theta);

    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float prev_t = 0.0;
    float3 in_scatter = 0.0;
    float3 optical_depth = 0.0;
    float3 throughput = 1.0;
    for (int i = 0; i < constant.sample_count; ++i)
    {
        float curr_t = distance * (float(i) + 0.3) / constant.sample_count;
        float dt = curr_t - prev_t;
        prev_t = curr_t;

        float3 position = eye + view * curr_t;

        float r = length(position);
        float mu = dot(normalize(position), -constant.sun_direction);

        float h = r - atmosphere.planet_radius;

        float3 rayleigh_scattering = atmosphere.get_rayleigh_scattering(h);
        float mie_scattering = atmosphere.get_mie_scattering(h);

        float3 extinction = 0.0;
        extinction += rayleigh_scattering;
        extinction += mie_scattering + atmosphere.get_mie_absorption(h);
        extinction += atmosphere.get_ozone_absorption(h);
        optical_depth += extinction * dt;

        float2 uv;
        get_transmittance_lut_uv(atmosphere.planet_radius, atmosphere.atmosphere_radius, r, mu, uv);

        float3 t0 = transmittance_lut.SampleLevel(linear_clamp_sampler, uv, 0.0);
        float3 s = rayleigh_scattering * rayleigh_phase + mie_scattering * mie_phase;
        float3 t1 = exp(-optical_depth);

        in_scatter += t0 * s * t1 * dt;
        throughput *= t1;
    }

    return float4(in_scatter * constant.sun_irradiance, dot(throughput, 1.0 / 3.0));
}

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture3D<float4> aerial_perspective_lut = ResourceDescriptorHeap[constant.aerial_perspective_lut];

    uint width;
    uint height;
    uint depth;
    aerial_perspective_lut.GetDimensions(width, height, depth);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    atmosphere_data atmosphere = constant.atmosphere;

    Texture2D<float3> transmittance_lut = ResourceDescriptorHeap[constant.transmittance_lut];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float3 eye = float3(0.0, max(camera.position.y + atmosphere.planet_radius, atmosphere.planet_radius + 1.0), 0.0);

    float2 uv = get_compute_texcoord(dtid.xy, width, height);
    float3 view = normalize(lerp(
        lerp(constant.frustum_top_left, constant.frustum_top_right, uv.x),
        lerp(constant.frustum_bottom_left, constant.frustum_bottom_right, uv.x), uv.y));

    float plant_distance = ray_sphere_intersection(eye, view, 0.0, atmosphere.planet_radius);
    float atmosphere_distance = ray_sphere_intersection(eye, view, 0.0, atmosphere.atmosphere_radius);

    float max_distance = 0.0;
    if (plant_distance < 0.0)
    {
        max_distance = atmosphere_distance < 0.0 ? 0.0 : atmosphere_distance;
    }
    else
    {
        max_distance = min(plant_distance, atmosphere_distance);
    }

    for (uint slice_id = 0; slice_id < depth; ++slice_id)
    {
        float slice = (float(slice_id) + 0.5) / depth;
        slice *= slice;
        slice *= depth;

        float slice_end = min(max_distance, slice * constant.distance_per_slice);
        if (slice_end <= 0.0)
        {
            aerial_perspective_lut[uint3(dtid.x, dtid.y, slice_id)] = 0.0;
        }
        else
        {
            aerial_perspective_lut[uint3(dtid.x, dtid.y, slice_id)] = integrate(atmosphere, slice_end, eye, view, transmittance_lut);
        }
    }
}