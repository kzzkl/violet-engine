#include "graphics/shadow_map.hpp"
#include "graphics/rhi.hpp"

namespace violet::graphics
{
shadow_map_pipeline_parameter::shadow_map_pipeline_parameter() : pipeline_parameter(layout)
{
}

void shadow_map_pipeline_parameter::light_view_projection(const math::float4x4& view_projection)
{
    field<constant_data>(0).light_view_projection = math::matrix::transpose(view_projection);
}

shadow_map::shadow_map(std::uint32_t resolution) : m_parameter(nullptr)
{
    shadow_map_desc desc;
    desc.width = desc.height = resolution;
    desc.samples = 1;
    m_shadow_map = rhi::make_shadow_map(desc);

    m_parameter = std::make_unique<shadow_map_pipeline_parameter>();
}

void shadow_map::light_view_projection(const math::float4x4& view_projection)
{
    m_parameter->light_view_projection(view_projection);
}
} // namespace violet::graphics