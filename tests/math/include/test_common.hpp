#pragma once

#include "math/math.hpp"
#include <catch2/catch_all.hpp>

namespace violet::test
{
bool equal(float a, float b);
bool equal(const violet::math::float2& a, const violet::math::float2& b);
bool equal(const violet::math::float3& a, const violet::math::float3& b);
bool equal(const violet::math::float4& a, const violet::math::float4& b);
bool equal(const violet::math::float4_simd& a, const violet::math::float4_simd& b);

bool equal(const violet::math::float4x4& a, const violet::math::float4x4& b);
bool equal(const violet::math::float4x4_simd& a, const violet::math::float4x4_simd& b);
} // namespace violet::test