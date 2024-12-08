#ifndef BRDF_HLSLI
#define BRDF_HLSLI

#include "common.hlsli"

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

float3 importance_sample_ggx(float2 xi, float3 n, float roughness)
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
    float3 up = abs(n.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, n));
    float3 bitangent = cross(n, tangent);

    float3 sample_vec = tangent * h.x + bitangent * h.y + n * h.z;
    return normalize(sample_vec);
}

float g_smith(float3 n, float3 v, float3 l, float roughness)
{
    float k = (roughness * roughness) / 2.0;

    float NdotV = saturate(dot(n, v));
    float NdotL = saturate(dot(n, l));
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k + EPSILON);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k + EPSILON);

    return ggx1 * ggx2;
}

float v_smith_joint_approx(float NdotV, float NdotL, float roughness)
{
    float a = roughness * roughness;
    float smith_v = NdotL * (NdotV * (1.0 - a) + a);
    float smith_l = NdotV * (NdotL * (1.0 - a) + a);
    return 0.5 * rcp(smith_v + smith_l + EPSILON);
}

float d_ggx(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / denom;
}

float3 f_schlick(float VdotH, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
}

float3 f_schlick_roughness(float VdotH, float3 F0, float roughness)
{
    return F0 + (max((1.0 - roughness).xxx, F0) - F0) * pow(1.0 - VdotH, 5.0);
}
#endif