#pragma once

#include "math/math.hpp"
#include <catch2/catch.hpp>

namespace ash::test
{
bool equal(float a, float b);
bool equal(const ash::math::float2& a, const ash::math::float2& b);
bool equal(const ash::math::float3& a, const ash::math::float3& b);
bool equal(const ash::math::float4& a, const ash::math::float4& b);
bool equal(const ash::math::float4_simd& a, const ash::math::float4_simd& b);

bool equal(const ash::math::float4x4& a, const ash::math::float4x4& b);
bool equal(const ash::math::float4x4_simd& a, const ash::math::float4x4_simd& b);
} // namespace ash::test