#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct atmosphere_data
{
    float3 rayleigh_scattering;
    float rayleigh_density_height;
    float mie_scattering;
    float mie_asymmetry;
    float mie_absorption;
    float mie_density_height;
    float3 ozone_absorption;
    float ozone_center_height;
    float ozone_width;
    float planet_radius;
    float atmosphere_radius;
    float sun_angular_radius;

    float3 get_rayleigh_scattering(float h)
    {
        return rayleigh_scattering * exp(-h / rayleigh_density_height);
    }

    float get_rayleigh_phase(float cos_theta)
    {
        return 3.0 / (16.0 * PI) * (1.0 + cos_theta * cos_theta);
    }

    float get_mie_scattering(float h)
    {
        return mie_scattering * exp(-h / mie_density_height);
    }

    float get_mie_absorption(float h)
    {
        return mie_absorption * exp(-h / mie_density_height);
    }

    float get_mie_extinction(float h)
    {
        float rho = exp(-h / mie_density_height);
        return (mie_scattering + mie_absorption) * rho;
    }

    float get_mie_phase(float cos_theta)
    {
        float g = mie_asymmetry;
        float g2 = g * g;
        
        float a = 3.0 / (8.0 * PI);
        float b = (1.0 - g2) / (2.0 + g2);
        float c = 1.0 + cos_theta * cos_theta;
        float d = pow(1.0 + g2 - 2.0 * g * cos_theta, 1.5);

        return a * b * c / d;
    }

    float3 get_ozone_absorption(float h)
    {
        return ozone_absorption * max(0.0, 1.0 - (abs(h - ozone_center_height) / ozone_width));
    }
};

float ray_sphere_intersection(float3 ray_origin, float3 ray_dir, float3 sphere_center, float sphere_radius)
{
    float3 oc = ray_origin - sphere_center;

    float b = dot(oc, ray_dir);
    float c = dot(oc, oc) - sphere_radius * sphere_radius;

    float h = b * b - c;
    if (h < 0.0)
    {
        return -1.0;
    }

    float sqrt_h = sqrt(h);

    float t1 = -b - sqrt_h;
    if (t1 > 0.0)
    {
        return t1;
    }

    float t2 = -b + sqrt_h;
    if (t2 > 0.0)
    {
        return t2;
    }

    return -1.0;
}

float ray_sphere_intersection(float radius, float r, float mu)
{
    float discriminant = r * r * (mu * mu - 1.0) + radius * radius;
    return max(0.0, (-r * mu + sqrt(discriminant)));
}

void get_transmittance_lut_r_mu(float planet_radius, float atmosphere_radius, float2 uv, out float r, out float mu)
{
    float x_mu = uv.x;
    float x_r = uv.y;

    float H = sqrt(atmosphere_radius * atmosphere_radius - planet_radius * planet_radius);
    float rho = H * x_r;
    r = sqrt(rho * rho + planet_radius * planet_radius);

    float d_min = atmosphere_radius - r;
    float d_max = rho + H;
    float d = d_min + x_mu * (d_max - d_min);
    mu = d == 0.0 ? 1.0 : (H * H - rho * rho - d * d) / (2.0 * r * d);
    mu = clamp(mu, -1.0, 1.0);
}

void get_transmittance_lut_uv(float planet_radius, float atmosphere_radius, float r, float mu, out float2 uv)
{
    float H = sqrt(atmosphere_radius * atmosphere_radius - planet_radius * planet_radius);
    float rho = sqrt(r * r - planet_radius * planet_radius);

    float d = ray_sphere_intersection(atmosphere_radius, r, mu);

    float d_min = atmosphere_radius - r;
    float d_max = rho + H;

    float x_mu = (d - d_min) / (d_max - d_min);
    float x_r = rho / H;

    uv = float2(x_mu, x_r);
}

float3 get_sky_view_lut_direction(float2 uv)
{
    float phi = (uv.x - 0.5) * TWO_PI;

    float v = (uv.y - 0.5) * 2.0;
    float theta = sign(v) * v * v * HALF_PI;

    float cos_theta = cos(theta);
    float sin_theta = sin(theta);
    float cos_phi = cos(phi);
    float sin_phi = sin(phi);

    float3 direction;
    direction.x = cos_theta * sin_phi;
    direction.y = sin_theta;
    direction.z = cos_theta * cos_phi;

    return normalize(direction);
}

float2 get_sky_view_lut_uv(float3 direction)
{
    float2 uv;
    uv.x = atan2(direction.x, direction.z) / TWO_PI + 0.5;

    float theta = asin(direction.y);
    uv.y = 0.5 + 0.5 * sign(theta) * sqrt(abs(theta) / HALF_PI);

    return uv;
}

bool move_to_atmosphere(inout float3 eye, float3 view, float atmosphere_radius)
{
    float height = length(eye);
    if (height < atmosphere_radius)
    {
        return true;
    }

    float atmosphere_distance = ray_sphere_intersection(eye, view, 0.0, atmosphere_radius);
    if (atmosphere_distance < 0.0)
    {
        return false;
    }

    eye = eye + view * atmosphere_distance - eye / height;
    return true;
}

float3 sun_disk(float3 view_dir, float3 sun_dir, float3 transmittance, float sun_angular_radius)
{
    float cos_theta = dot(view_dir, -sun_dir);
    float cos_radius = cos(sun_angular_radius);

    return smoothstep(cos_radius, cos_radius + 0.01, cos_theta) * 1e9 * transmittance;
}

float get_shadow(float3 camera, float3 position, uint vsm_id, StructuredBuffer<vsm_data> vsms, StructuredBuffer<uint> virtual_page_table, Texture2D<uint> physical_shadow_map)
{
    uint cascade = get_directional_cascade(length(position - camera));
    vsm_data vsm = vsms[vsm_id + cascade];

    float4 position_ls = mul(vsm.matrix_vp, float4(position, 1.0));
    position_ls /= position_ls.w;
    position_ls.xy = position_ls.xy * 0.5 + 0.5;

    float shadow_depth;
    bool valid = sample_shadow_depth(vsm_id + cascade, position_ls.xy, physical_shadow_map, virtual_page_table, shadow_depth);
    return valid ? (shadow_depth > position_ls.z ? 0.0 : 1.0) : 1.0;
}

float4 integrate_atmosphere(
    atmosphere_data atmosphere,
    float3 eye,
    float3 view,
    float3 sun_direction,
    float distance,
    float sample_count,
    Texture2D<float3> transmittance_lut
#ifdef USE_MULTI_SCATTERING
    ,
    Texture2D<float3> multi_scattering_lut
#endif
#ifdef USE_SHADOW
    ,
    uint vsm_id,
    StructuredBuffer<vsm_data> vsms,
    StructuredBuffer<uint> virtual_page_table,
    Texture2D<uint> physical_shadow_map
#endif
    )
{
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float dt = distance / sample_count;

    float cos_theta = dot(-view, sun_direction);
    float3 rayleigh_phase = atmosphere.get_rayleigh_phase(cos_theta);
    float mie_phase = atmosphere.get_mie_phase(cos_theta);

    float3 p = eye + view * dt * 0.5;
    float3 in_scatter = 0.0;
    float3 optical_depth = 0.0;
    float3 throughput = 1.0;
    for (int i = 0; i < sample_count; ++i)
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

        float3 t1 = exp(-optical_depth);
        if (ray_sphere_intersection(p, -sun_direction, 0.0, atmosphere.planet_radius) < 0.0)
        {
            float shadow = 1.0;

#ifdef USE_SHADOW
            shadow = get_shadow(
                eye - float3(0.0, atmosphere.planet_radius, 0.0),
                p - float3(0.0, atmosphere.planet_radius, 0.0),
                vsm_id,
                vsms,
                virtual_page_table,
                physical_shadow_map);
#endif

            float2 uv;
            get_transmittance_lut_uv(atmosphere.planet_radius, atmosphere.atmosphere_radius, r, mu, uv);
            float3 t0 = transmittance_lut.SampleLevel(linear_clamp_sampler, uv, 0.0);
            float3 s = rayleigh_scattering * rayleigh_phase + mie_scattering * mie_phase;
            in_scatter += t0 * s * t1 * dt * shadow;
        }
        throughput *= t1;

#ifdef USE_MULTI_SCATTERING
        float2 multi_scattering_uv;
        multi_scattering_uv.x = mu * 0.5 + 0.5;
        multi_scattering_uv.y = min(h / (atmosphere.atmosphere_radius - atmosphere.planet_radius), 1.0);
        float3 multi_scattering = multi_scattering_lut.SampleLevel(linear_clamp_sampler, multi_scattering_uv, 0.0);
        in_scatter += multi_scattering * (rayleigh_scattering + mie_scattering) * t1 * dt;
#endif

        p += view * dt;
    }

    return float4(in_scatter, dot(throughput, 1.0 / 3.0));
}