#include "graphics/light.hpp"
#include "assert.hpp"

namespace ash::graphics
{
directional_light::directional_light() : m_color{1.0f, 1.0f, 1.0f}
{
    m_shadow_map = std::make_unique<shadow_map>(2048);
}

light_pipeline_parameter::light_pipeline_parameter() : pipeline_parameter("ash_light")
{
}

void light_pipeline_parameter::directional_light(
    std::size_t index,
    const math::float3& color,
    const math::float3& direction)
{
    ASH_ASSERT(index < MAX_DIRECTIONAL_LIGHT_COUNT);

    auto& parameter = field<constant_data>(0).directional_lights[index];
    parameter.color = color;
    parameter.direction = direction;
}

void light_pipeline_parameter::directional_light_count(std::size_t count)
{
    ASH_ASSERT(count < MAX_DIRECTIONAL_LIGHT_COUNT);

    field<constant_data>(0).directional_light_count = count;
}

std::vector<pipeline_parameter_pair> light_pipeline_parameter::layout()
{
    return {
        {PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(constant_data)}
    };
}
} // namespace ash::graphics