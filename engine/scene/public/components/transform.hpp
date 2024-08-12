#pragma once

#include "math/math.hpp"

namespace violet
{
struct transform
{
    float3 position{0.0f, 0.0f, 0.0f};
    float4 rotation{0.0f, 0.0f, 0.0f, 1.0f};
    float3 scale{1.0f, 1.0f, 1.0f};
};

struct transform_local
{
    float4x4 matrix;
};

struct transform_world
{
    float4x4 matrix;
};
} // namespace violet