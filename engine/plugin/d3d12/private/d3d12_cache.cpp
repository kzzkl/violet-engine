#include "d3d12_cache.hpp"

namespace violet::graphics::d3d12
{
d3d12_frame_buffer* d3d12_cache::get_or_create_frame_buffer(
    d3d12_render_pipeline* pipeline,
    const d3d12_camera_info& camera_info)
{
    auto& result = m_frame_buffers[camera_info];
    if (result == nullptr)
        result = std::make_unique<d3d12_frame_buffer>(pipeline, camera_info);

    return result.get();
}

d3d12_pipeline_parameter_layout* d3d12_cache::get_or_create_pipeline_parameter_layout(
    const pipeline_parameter_desc& desc)
{
    auto& result = m_pipeline_parameter_layouts[desc];
    if (result == nullptr)
        result = std::make_unique<d3d12_pipeline_parameter_layout>(desc);

    return result.get();
}

void d3d12_cache::on_resource_destroy(d3d12_resource* resource)
{
    for (auto iter = m_frame_buffers.begin(); iter != m_frame_buffers.end();)
    {
        if (iter->first.render_target == resource ||
            iter->first.render_target_resolve == resource ||
            iter->first.depth_stencil_buffer == resource)
            iter = m_frame_buffers.erase(iter);
        else
            ++iter;
    }
}
} // namespace violet::graphics::d3d12