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

shadow_map::shadow_map(std::size_t resolution) : m_parameter(nullptr)
{
    depth_stencil_buffer_info info = {};
    info.format = RESOURCE_FORMAT_D32_FLOAT;
    info.width = info.height = resolution;
    info.samples = 4;

    m_shadow_map = rhi::make_depth_stencil_buffer(info);

    render_target_info ti = {};
    ti.format = RESOURCE_FORMAT_R8G8B8A8_UNORM;
    ti.width = ti.height = resolution;
    ti.samples = 4;
    m_test = rhi::make_render_target(ti);
}
} // namespace ash::graphics