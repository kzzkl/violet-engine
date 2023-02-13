#include "graphics/light.hpp"
#include "assert.hpp"

namespace violet::graphics
{
directional_light::directional_light() : m_color{1.0f, 1.0f, 1.0f}
{
}

light_pipeline_parameter::light_pipeline_parameter() : pipeline_parameter(layout)
{
}

void light_pipeline_parameter::ambient_light(const math::float3& color)
{
    field<constant_data>(0).ambient_light = color;
}

void light_pipeline_parameter::directional_light(
    std::size_t index,
    const math::float3& color,
    const math::float3& direction,
    bool shadow,
    std::size_t shadow_index)
{
    VIOLET_ASSERT(index < MAX_DIRECTIONAL_LIGHT_COUNT);

    auto& parameter = field<constant_data>(0).directional_lights[index];
    parameter.color = color;
    parameter.direction = direction;
    parameter.shadow = shadow;
    parameter.shadow_index = static_cast<std::uint32_t>(shadow_index);
}

void light_pipeline_parameter::directional_light_count(std::size_t count)
{
    VIOLET_ASSERT(count < MAX_DIRECTIONAL_LIGHT_COUNT);

    field<constant_data>(0).directional_light_count = count;
}

void light_pipeline_parameter::shadow(
    std::size_t index,
    const math::float4x4& shadow_v,
    const std::array<math::float4, MAX_CASCADED_COUNT>& cascade_scale,
    const std::array<math::float4, MAX_CASCADED_COUNT>& cascade_offset)
{
    auto& parameter = field<constant_data>(0);

    parameter.shadow_v[index] = math::matrix::transpose(shadow_v);

    for (std::size_t i = 0; i < MAX_CASCADED_COUNT; ++i)
    {
        parameter.cascade_scale[index][i] = cascade_scale[i];
        parameter.cascade_offset[index][i] = cascade_offset[i];
    }
}

void light_pipeline_parameter::shadow_map(
    std::size_t index,
    std::size_t cascade,
    resource_interface* shadow_map)
{
    interface()->set(1, shadow_map, index * MAX_CASCADED_COUNT + cascade);
}

void light_pipeline_parameter::shadow_cascade_depths(const math::float4& cascade_depths)
{
    field<constant_data>(0).cascade_depths = cascade_depths;
}

void light_pipeline_parameter::shadow_count(std::size_t shadow_count, std::size_t cascade_count)
{
    auto& parameter = field<constant_data>(0);
    parameter.shadow_count = shadow_count;
    parameter.cascade_count = cascade_count;
}
} // namespace violet::graphics