#include "atmosphere/atmosphere.hlsli"

struct constant_data
{
    atmosphere_data atmosphere;

    uint transmittance_lut;
    uint multi_scattering_lut;
    uint sample_count;
    uint direction_count;
};
PushConstant(constant_data, constant);

static float3 random_direction[64] = {
    float3(0.176085, 0.0, 0.984375),         float3(-0.223111, -0.204388, 0.953125),
    float3(0.0338763, 0.386004, 0.921875),   float3(0.276681, -0.360881, 0.890625),
    float3(-0.503529, 0.089067, 0.859375),   float3(0.472962, 0.300859, 0.828125),
    float3(-0.156839, -0.583431, 0.796875),  float3(-0.296495, 0.570885, 0.765625),
    float3(0.637559, -0.232835, 0.734375),   float3(-0.657271, -0.271312, 0.703125),
    float3(0.313928, 0.670845, 0.671875),    float3(0.229806, -0.732659, 0.640625),
    float3(-0.68601, 0.397558, 0.609375),    float3(0.796917, 0.1752, 0.578125),
    float3(-0.481509, -0.684892, 0.546875),  float3(-0.110109, 0.84971, 0.515625),
    float3(0.668962, -0.5638, 0.484375),     float3(-0.890686, -0.0368311, 0.453125),
    float3(0.642663, 0.639536, 0.421875),    float3(-0.0425246, -0.919567, 0.390625),
    float3(-0.597905, 0.716491, 0.359375),   float3(0.936198, -0.125962, 0.328125),
    float3(-0.783852, -0.545381, 0.296875),  float3(0.211597, 0.940569, 0.265625),
    float3(0.48333, -0.843481, 0.234375),    float3(-0.932831, 0.2976, 0.203125),
    float3(0.894282, 0.413182, 0.171875),    float3(-0.382221, -0.913308, 0.140625),
    float3(-0.336416, 0.93534, 0.109375),    float3(0.882482, -0.463812, 0.078125),
    float3(-0.965907, -0.254609, 0.046875),  float3(0.540771, 0.841025, 0.015625),
    float3(0.169359, -0.985431, -0.015625),  float3(-0.78975, 0.611635, -0.046875),
    float3(0.99354, 0.0823088, -0.078125),   float3(-0.675006, -0.72966, -0.109375),
    float3(0.00482904, 0.990051, -0.140625), float3(0.661889, -0.729631, -0.171875),
    float3(-0.974974, 0.0903666, -0.203125), float3(0.774375, 0.587718, -0.234375),
    float3(-0.172556, -0.948508, -0.265625), float3(-0.508594, 0.808206, -0.296875),
    float3(0.911041, -0.249676, -0.328125),  float3(-0.830252, -0.426065, -0.359375),
    float3(0.320002, 0.86314, -0.390625),    float3(0.341845, -0.83974, -0.421875),
    float3(-0.805561, 0.38177, -0.453125),   float3(0.836027, 0.257758, -0.484375),
    float3(-0.433229, -0.739218, -0.515625), float3(-0.175771, 0.818555, -0.546875),
    float3(0.665198, -0.472528, -0.578125),  float3(-0.786795, -0.0980569, -0.609375),
    float3(0.497698, 0.584719, -0.640625),   float3(0.0269958, -0.740172, -0.671875),
    float3(-0.499111, 0.506462, -0.703125),  float3(0.677859, -0.0346529, -0.734375),
    float3(-0.495915, -0.40974, -0.765625),  float3(0.0834866, 0.598348, -0.796875),
    float3(0.317892, -0.461685, -0.828125),  float3(-0.498324, 0.114666, -0.859375),
    float3(0.395651, 0.224159, -0.890625),   float3(-0.11957, -0.368578, -0.921875),
    float3(-0.125567, 0.275292, -0.953125),  float3(0.1621, -0.0687703, -0.984375),
};

float3 integrate_multi_scattering(
    atmosphere_data atmosphere,
    float3 position,
    float3 sun_direction,
    Texture2D<float3> transmittance_lut,
    SamplerState linear_clamp_sampler)
{
    float3 g2 = 0.0;
    float3 fms = 0.0;

    for (uint i = 0; i < constant.direction_count; ++i)
    {
        float3 view = random_direction[i];
        
        float distance = ray_sphere_intersection(position, view, 0.0, atmosphere.planet_radius);
        if (distance < 0.0)
        {
            distance = ray_sphere_intersection(position, view, 0.0, atmosphere.atmosphere_radius);
        }

        float dt = distance / constant.sample_count;

        float cos_theta = dot(-view, sun_direction);
        float3 rayleigh_phase = atmosphere.get_rayleigh_phase(cos_theta);
        float mie_phase = atmosphere.get_mie_phase(cos_theta);

        float3 p = position + view * dt * 0.5;
        float3 optical_depth = 0.0;

        for (uint j = 0; j < constant.sample_count; ++j)
        {
            float r = length(p);
            float mu = dot(p / r, -sun_direction);

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

            g2 += t0 * s * t1 * dt;
            fms += t1 * (rayleigh_scattering + mie_scattering) * dt;

            p += view * dt;
        }
    }

    g2 /= constant.direction_count;
    fms /= constant.direction_count;
    return g2 / (1.0 - fms);
}

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float3> multi_scattering_lut = ResourceDescriptorHeap[constant.multi_scattering_lut];

    uint width;
    uint height;
    multi_scattering_lut.GetDimensions(width, height);

    float sun_cos = lerp(-1.0, 1.0, (float(dtid.x) + 0.5) / height);
    float3 sun_direction = float3(sqrt(1.0 - sun_cos * sun_cos), sun_cos, 0.0);

    atmosphere_data atmosphere = constant.atmosphere;

    float position_height = lerp(atmosphere.planet_radius, atmosphere.atmosphere_radius, (float(dtid.y) + 0.5) / width);
    float3 position = float3(0.0, position_height, 0.0);

    Texture2D<float3> transmittance_lut = ResourceDescriptorHeap[constant.transmittance_lut];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float3 multi_scattering = integrate_multi_scattering(atmosphere, position, -sun_direction, transmittance_lut, linear_clamp_sampler);
    multi_scattering_lut[dtid.xy] = multi_scattering;
}