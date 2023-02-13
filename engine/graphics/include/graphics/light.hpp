#pragma once

#include "graphics/pipeline.hpp"
#include <array>

namespace violet::graphics
{
struct point_light
{
};

class directional_light
{
public:
    directional_light();

    void color(const math::float3& color) noexcept { m_color = color; }
    const math::float3& color() const noexcept { return m_color; }

private:
    math::float3 m_color;
};

class light_pipeline_parameter : public pipeline_parameter
{
public:
    static constexpr std::size_t MAX_DIRECTIONAL_LIGHT_COUNT = 4;
    static constexpr std::size_t MAX_POINT_LIGHT_COUNT = 20;

    static constexpr std::size_t MAX_SHADOW_COUNT = 4;
    static constexpr std::size_t MAX_CASCADED_COUNT = 4;

    struct directional_light_data
    {
        math::float3 direction;
        std::uint32_t shadow;
        math::float3 color;
        std::uint32_t shadow_index;
    };

    struct constant_data
    {
        directional_light_data directional_lights[MAX_DIRECTIONAL_LIGHT_COUNT];
        std::uint32_t directional_light_count;
        math::float3 ambient_light;

        math::float4x4 shadow_v[MAX_SHADOW_COUNT];
        math::float4 cascade_depths;
        math::float4 cascade_scale[MAX_SHADOW_COUNT][MAX_CASCADED_COUNT];
        math::float4 cascade_offset[MAX_SHADOW_COUNT][MAX_CASCADED_COUNT];

        std::uint32_t shadow_count;
        std::uint32_t cascade_count;
    };

    static constexpr pipeline_parameter_desc layout = {
        .parameters = {
            {PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(constant_data)},
            {PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE, MAX_SHADOW_COUNT* MAX_CASCADED_COUNT}},
        .parameter_count = 2
    };

public:
    light_pipeline_parameter();

    void ambient_light(const math::float3& color);
    void directional_light(
        std::size_t index,
        const math::float3& color,
        const math::float3& direction,
        bool shadow,
        std::size_t shadow_index);
    void directional_light_count(std::size_t count);

    void shadow(
        std::size_t index,
        const math::float4x4& shadow_v,
        const std::array<math::float4, MAX_CASCADED_COUNT>& cascade_scale,
        const std::array<math::float4, MAX_CASCADED_COUNT>& cascade_offset);
    void shadow_map(std::size_t index, std::size_t cascade, resource_interface* shadow_map);
    void shadow_cascade_depths(const math::float4& cascade_depths);
    void shadow_count(std::size_t shadow_count, std::size_t cascade_count);
};
} // namespace violet::graphics