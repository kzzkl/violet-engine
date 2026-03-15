#ifndef SPHERICAL_HARMONICS_HLSLI
#define SPHERICAL_HARMONICS_HLSLI

#include "common.hlsli"

static const float BASIS_L0 = 0.5 * sqrt(1.0 / PI);
static const float BASIS_L1 = 0.5 * sqrt(3.0 / PI);
static const float BASIS_L2_MN2 = 0.5 * sqrt(15.0 / PI);
static const float BASIS_L2_MN1 = 0.5 * sqrt(15.0 / PI);
static const float BASIS_L2_M0 = 0.25 * sqrt(5.0 / PI);
static const float BASIS_L2_M1 = 0.5 * sqrt(15.0 / PI);
static const float BASIS_L2_M2 = 0.25 * sqrt(15.0 / PI);

struct sh9
{
    float4 c[9];

    static void get_basis(float3 direction, out float basis[9])
    {
        float x = direction.x;
        float y = direction.y;
        float z = direction.z;

        basis[0] = BASIS_L0;
        basis[1] = BASIS_L1 * y;
        basis[2] = BASIS_L1 * z;
        basis[3] = BASIS_L1 * x;
        basis[4] = BASIS_L2_MN2 * x * y;
        basis[5] = BASIS_L2_MN1 * y * z;
        basis[6] = BASIS_L2_M0 * (3.0 * z * z - 1.0);
        basis[7] = BASIS_L2_M1 * x * z;
        basis[8] = BASIS_L2_M2 * (x * x - y * y);
    }

    void add(sh9 other)
    {
        for (int i = 0; i < 9; ++i)
        {
            c[i] += other.c[i];
        }
    }

    void project(float3 direction, float3 value)
    {
        float basis[9];
        get_basis(direction, basis);

        for (int i = 0; i < 9; ++i)
        {
            c[i].xyz += value * basis[i];
        }
    }

    float3 evaluate(float3 direction)
    {
        const float c1 = 0.429043;
        const float c2 = 0.511664;
        const float c3 = 0.743125;
        const float c4 = 0.886227;
        const float c5 = 0.247708;

        float x = direction.x;
        float y = direction.y;
        float z = direction.z;

        float3 irradiance =
            c1 * c[8].xyz * (x * x - y * y) +
            c3 * c[6].xyz * z * z +
            c4 * c[0].xyz -
            c5 * c[6].xyz +
            2.0 * c1 * c[4].xyz * x * y +
            2.0 * c1 * c[7].xyz * x * z +
            2.0 * c1 * c[5].xyz * y * z +
            2.0 * c2 * c[3].xyz * x +
            2.0 * c2 * c[1].xyz * y +
            2.0 * c2 * c[2].xyz * z;

        return max(irradiance, 0.0);
    }
};

#endif