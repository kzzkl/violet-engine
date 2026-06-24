#pragma once

#include "graphics/resources/texture.hpp"

namespace violet
{
class distance_field_tool
{
public:
    static bool generate(
        std::span<const vec3f> positions,
        std::span<const std::uint32_t> indexes,
        texture_data& distance_field);

    static bool test(
        std::span<const vec3f> positions,
        std::span<const std::uint32_t> indexes,
        texture_data& distance_field);
};
} // namespace violet