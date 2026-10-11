#pragma once

#include "graphics/distance_field/distance_field.hpp"

namespace violet
{
class distance_field_tool
{
public:
    struct input
    {
        std::span<const vec3f> positions;
        std::span<const std::uint32_t> indexes;

        float voxel_density{20.0f};
    };

    using output = distance_field;

    static output generate(const input& input);

    static bool test(const input& input);
};
} // namespace violet