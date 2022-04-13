#pragma once

#include "math.hpp"
#include <vector>

namespace ash::sample::mmd
{
struct bone
{
    math::float4x4 offset;
    math::float4x4 transform;
};
} // namespace ash::sample::mmd