#ifndef VIOLET_BRDF_INCLUDE
#define VIOLET_BRDF_INCLUDE

#include "violet_define.hlsli"

float radical_inverse_vdc(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 hammersley(uint i, uint n)
{
    return float2(float(i) / float(n), radical_inverse_vdc(i));
}

float3 importance_sample_ggx(float2 xi, float3 normal, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * xi.x;
    float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    // from spherical coordinates to cartesian coordinates
    float3 h;
    h.x = cos(phi) * sin_theta;
    h.y = sin(phi) * sin_theta;
    h.z = cos_theta;

    // from tangent-space vector to world-space sample vector
    float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);

    float3 sample_vec = tangent * h.x + bitangent * h.y + normal * h.z;
    return normalize(sample_vec);
}

float geometry_schlick_ggx(float n_dot_v, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = n_dot_v;
    float denom = n_dot_v * (1.0 - k) + k;

    return nom / denom;
}

float geometry_smith(float3 normal, float3 view, float3 light, float roughness)
{
    float n_dot_v = max(dot(normal, view), 0.0);
    float n_dot_l = max(dot(normal, light), 0.0);
    float ggx2 = geometry_schlick_ggx(n_dot_v, roughness);
    float ggx1 = geometry_schlick_ggx(n_dot_l, roughness);

    return ggx1 * ggx2;
}  

#endif