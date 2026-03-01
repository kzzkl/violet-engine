#include "common.hlsli"

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

struct atmosphere
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
    float atmosphere_height;

    float3 get_rayleigh_coefficient(float h)
    {
        float rho_h = exp(-h / rayleigh_density_height);
        return rayleigh_scattering * rho_h;
    }

    float get_rayleigh_phase(float cos_theta)
    {
        return 3.0 / (16.0 * PI) * (1.0 + cos_theta * cos_theta);
    }

    float3 get_mie_coefficient(float h)
    {
        float rho_h = exp(-h / mie_density_height);
        return mie_scattering * rho_h;
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

    float3 get_mie_absorption(float h)
    {
        float rho_h = exp(-h / mie_density_height);
        return mie_absorption * rho_h;
    }

    float3 get_ozone_absorption(float h)
    {
        float rho = max(0.0, 1.0 - (abs(h - ozone_center_height) / ozone_width));
        return ozone_absorption * rho;
    }

    float3 scattering(float3 p, float3 in_dir, float3 out_dir)
    {
        float cos_theta = dot(in_dir, out_dir);

        float h = length(p) - planet_radius;
        float3 rayleigh = get_rayleigh_coefficient(h) * get_rayleigh_phase(cos_theta);
        float3 mie = get_mie_coefficient(h) * get_mie_phase(cos_theta);
        
        return rayleigh + mie;
    }

    float3 sun_disk(float3 view_dir, float3 sun_dir, float3 transmittance, float sun_angular_radius = 0.00465 * 2)
    {
        float cos_theta = dot(view_dir, -sun_dir);
        float cos_radius = cos(sun_angular_radius);
        return smoothstep(cos_radius, cos_radius + 0.02, cos_theta) * 1e9 * transmittance;
    }
};