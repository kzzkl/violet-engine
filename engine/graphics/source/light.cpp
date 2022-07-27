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
    const math::float4x4& light_vp,
    resource_interface* shadow_map)
{
    ASH_ASSERT(index < MAX_DIRECTIONAL_LIGHT_COUNT);

    auto& parameter = field<constant_data>(0).directional_lights[index];
    parameter.color = color;
    parameter.direction = direction;
    parameter.light_vp = math::matrix::transpose(light_vp);

    interface()->set(1, shadow_map);
}

void light_pipeline_parameter::directional_light_count(std::size_t count)
{
    ASH_ASSERT(count < MAX_DIRECTIONAL_LIGHT_COUNT);

    field<constant_data>(0).directional_light_count = count;
}

std::vector<pipeline_parameter_pair> light_pipeline_parameter::layout()
{
    return {
        {PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(constant_data)}, // Light attributes
        {PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE, 1                    }  // Shadow map
    };
}
} // namespace ash::graphics