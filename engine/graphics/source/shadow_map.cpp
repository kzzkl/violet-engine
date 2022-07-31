#include "graphics/shadow_map.hpp"
#include "graphics/rhi.hpp"

namespace ash::graphics
{
shadow_map_pipeline_parameter::shadow_map_pipeline_parameter() : pipeline_parameter("ash_shadow")
{
}

void shadow_map_pipeline_parameter::light_view_projection(const math::float4x4& view_projection)
{
    field<constant_data>(0).light_view_projection = math::matrix::transpose(view_projection);
}

std::vector<pipeline_parameter_pair> shadow_map_pipeline_parameter::layout()
{
    return {
        {PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER,
         sizeof(shadow_map_pipeline_parameter::constant_data)}
    };
}

shadow_map::shadow_map(std::uint32_t resolution) : m_parameter(nullptr)
{
    shadow_map_info info;
    info.width = info.height = resolution;
    info.samples = 1;
    m_shadow_map = rhi::make_shadow_map(info);

    m_parameter = std::make_unique<shadow_map_pipeline_parameter>();
}

void shadow_map::light_view_projection(const math::float4x4& view_projection)
{
    m_parameter->light_view_projection(view_projection);
}
} // namespace ash::graphics