#pragma once

#include "math/math.hpp"
#include <catch2/catch_all.hpp>

namespace violet::test
{
bool equal(float a, float b);
bool equal(const violet::vec2f& a, const violet::vec2f& b);
bool equal(const violet::vec3f& a, const violet::vec3f& b);
bool equal(const violet::vec4f& a, const violet::vec4f& b);
bool equal(const violet::mat4f& a, const violet::mat4f& b);
bool equal(const violet::vec4f_simd& a, const violet::vec3f& b);
bool equal(const violet::vec4f_simd& a, const violet::vec4f& b);
bool equal(const violet::mat4f_simd& a, const violet::mat4f& b);
} // namespace violet::test