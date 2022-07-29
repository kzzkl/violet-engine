#include "graphics/light.hpp"
#include "assert.hpp"

namespace ash::graphics
{
directional_light::directional_light() : m_color{1.0f, 1.0f, 1.0f}
{
}

light_pipeline_parameter::light_pipeline_parameter() : pipeline_parameter("ash_light")
{
}

void light_pipeline_parameter::directional_light(
    std::size_t index,
    const math::float3& color,
    const math::float3& direction,
    bool shadow,
    std::size_t shadow_index)
{
    ASH_ASSERT(index < MAX_DIRECTIONAL_LIGHT_COUNT);

    auto& parameter = field<constant_data>(0).directional_lights[index];
    parameter.color = color;
    parameter.direction = direction;
    parameter.shadow = shadow;
    parameter.shadow_index = static_cast<std::uint32_t>(shadow_index);
}

void light_pipeline_parameter::directional_light_count(std::size_t count)
{
    ASH_ASSERT(count < MAX_DIRECTIONAL_LIGHT_COUNT);

    field<constant_data>(0).directional_light_count = count;
}

void light_pipeline_parameter::shadow(
    std::size_t index,
    const math::float4x4& shadow_v,
    const std::array<math::float4, MAX_CASCADED_COUNT>& cascaded_scale,
    const std::array<math::float4, MAX_CASCADED_COUNT>& cascaded_offset)
{
    auto& parameter = field<constant_data>(0);

    parameter.shadow_v[index] = math::matrix::transpose(shadow_v);

    for (std::size_t i = 0; i < MAX_CASCADED_COUNT; ++i)
    {
        parameter.cascaded_scale[index][i] = cascaded_scale[i];
        parameter.cascaded_offset[index][i] = cascaded_offset[i];
    }
}

void light_pipeline_parameter::shadow_map(
    std::size_t index,
    std::size_t cascaded,
    resource_interface* shadow_map)
{
    interface()->set(1 + index * MAX_CASCADED_COUNT + cascaded, shadow_map);
}

void light_pipeline_parameter::shadow_count(std::size_t shadow_count, std::size_t cascaded_count)
{
    auto& parameter = field<constant_data>(0);
    parameter.shadow_count = shadow_count;
    parameter.cascaded_count = cascaded_count;
}

std::vector<pipeline_parameter_pair> light_pipeline_parameter::layout()
{
    const std::size_t shadow_map_count = MAX_SHADOW_COUNT * MAX_CASCADED_COUNT;
    return {
        {PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(constant_data)}, // Light attributes
        {PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE, shadow_map_count     }  // Shadow map
    };
}
} // namespace ash::graphics