#include "vk_deletion_queue.hpp"
#include "vk_context.hpp"

namespace violet::vk
{
vk_deletion_queue::vk_deletion_queue(vk_context* context) noexcept
    : m_context(context)
{
}

void vk_deletion_queue::tick(std::size_t frame_index)
{
    m_render_pass_queue.tick(frame_index);
    m_raster_pipeline_queue.tick(frame_index);
    m_compute_pipeline_queue.tick(frame_index);
    m_parameter_queue.tick(frame_index);
    m_descriptor_queue.tick(frame_index);
    m_buffer_queue.tick(frame_index);
    m_texture_queue.tick(frame_index);
    m_swapchain_queue.tick(frame_index);
    m_fence_queue.tick(frame_index);
    m_query_pool_queue.tick(frame_index);
}

void vk_deletion_queue::flush()
{
    m_render_pass_queue.flush();
    m_raster_pipeline_queue.flush();
    m_compute_pipeline_queue.flush();
    m_parameter_queue.flush();
    m_descriptor_queue.flush();
    m_buffer_queue.flush();
    m_texture_queue.flush();
    m_swapchain_queue.flush();
    m_fence_queue.flush();
    m_query_pool_queue.flush();
}

void vk_deletion_queue::push(rhi_render_pass* render_pass)
{
    m_render_pass_queue.push(render_pass, get_delete_frame());
}

void vk_deletion_queue::push(rhi_raster_pipeline* raster_pipeline)
{
    m_raster_pipeline_queue.push(raster_pipeline, get_delete_frame());
}

void vk_deletion_queue::push(rhi_compute_pipeline* compute_pipeline)
{
    m_compute_pipeline_queue.push(compute_pipeline, get_delete_frame());
}

void vk_deletion_queue::push(rhi_parameter* parameter)
{
    m_parameter_queue.push(parameter, get_delete_frame());
}

void vk_deletion_queue::push(rhi_texture_srv* srv)
{
    m_descriptor_queue.push(srv, get_delete_frame());
}

void vk_deletion_queue::push(rhi_texture_uav* uav)
{
    m_descriptor_queue.push(uav, get_delete_frame());
}

void vk_deletion_queue::push(rhi_texture_rtv* rtv)
{
    m_descriptor_queue.push(rtv, get_delete_frame());
}

void vk_deletion_queue::push(rhi_texture_dsv* dsv)
{
    m_descriptor_queue.push(dsv, get_delete_frame());
}

void vk_deletion_queue::push(rhi_buffer_srv* srv)
{
    m_descriptor_queue.push(srv, get_delete_frame());
}

void vk_deletion_queue::push(rhi_buffer_uav* uav)
{
    m_descriptor_queue.push(uav, get_delete_frame());
}

void vk_deletion_queue::push(rhi_sampler* sampler)
{
    m_descriptor_queue.push(sampler, get_delete_frame());
}

void vk_deletion_queue::push(rhi_buffer* buffer)
{
    m_buffer_queue.push(buffer, get_delete_frame());
}

void vk_deletion_queue::push(rhi_texture* texture)
{
    m_texture_queue.push(texture, get_delete_frame());
}

void vk_deletion_queue::push(rhi_swapchain* swapchain)
{
    m_swapchain_queue.push(swapchain, get_delete_frame());
}

void vk_deletion_queue::push(rhi_fence* fence)
{
    m_fence_queue.push(fence, get_delete_frame());
}

void vk_deletion_queue::push(rhi_query_pool* query_pool)
{
    m_query_pool_queue.push(query_pool, get_delete_frame());
}

std::size_t vk_deletion_queue::get_delete_frame() const noexcept
{
    return m_context->get_frame_resource_index() + m_context->get_frame_resource_count();
}
} // namespace violet::vk