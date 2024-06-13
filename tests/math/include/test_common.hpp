#pragma once

#include "math/math.hpp"
#include <catch2/catch_all.hpp>

namespace violet::test
{
bool equal(float a, float b);
bool equal(const violet::float2& a, const violet::float2& b);
bool equal(const violet::float3& a, const violet::float3& b);
bool equal(const violet::float4& a, const violet::float4& b);
bool equal(const violet::float4x4& a, const violet::float4x4& b);

#ifdef VIOLET_USE_SIMD
bool equal(const violet::vector4& a, const violet::vector4& b);
bool equal(const violet::matrix4& a, const violet::matrix4& b);
#endif
} // namespace violet::test