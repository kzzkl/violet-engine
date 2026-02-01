#include "transient_allocator.hpp"

namespace violet
{
rhi_parameter* transient_allocator::allocate_parameter(const rhi_parameter_desc& desc)
{
    return m_parameter_allocator.allocate(
        desc,
        [](const rhi_parameter_desc& desc)
        {
            return render_device::instance().create_parameter(desc);
        });
}

rhi_texture* transient_allocator::allocate_texture(const rhi_texture_desc& desc)
{
    assert(desc.layout == RHI_TEXTURE_LAYOUT_UNDEFINED);

    return m_texture_allocator.allocate(
        desc,
        [](const rhi_texture_desc& desc)
        {
            return render_device::instance().create_texture(desc);
        });
}

void transient_allocator::free_texture(rhi_texture* texture)
{
    rhi_texture_desc desc = {};
    desc.extent = texture->get_extent();
    desc.format = texture->get_format();
    desc.flags = texture->get_flags();
    desc.level_count = texture->get_level_count();
    desc.layer_count = texture->get_layer_count();
    desc.samples = texture->get_samples();
    m_texture_allocator.free(desc, texture);
}

rhi_buffer* transient_allocator::allocate_buffer(const rhi_buffer_desc& desc)
{
    return m_buffer_allocator.allocate(
        desc,
        [](const rhi_buffer_desc& desc)
        {
            return render_device::instance().create_buffer(desc);
        });
}

void transient_allocator::free_buffer(rhi_buffer* buffer)
{
    rhi_buffer_desc desc = {};
    desc.size = buffer->get_size();
    desc.flags = buffer->get_flags();
    m_buffer_allocator.free(desc, buffer);
}

rhi_render_pass* transient_allocator::get_render_pass(const rhi_render_pass_desc& desc)
{
    return m_render_pass_allocator.allocate(
        desc,
        [](const rhi_render_pass_desc& desc)
        {
            return render_device::instance().create_render_pass(desc);
        });
}

rhi_raster_pipeline* transient_allocator::get_pipeline(const rhi_raster_pipeline_desc& desc)
{
    return m_raster_pipeline_allocator.allocate(
        desc,
        [](const rhi_raster_pipeline_desc& desc)
        {
            return render_device::instance().create_pipeline(desc);
        });
}

rhi_compute_pipeline* transient_allocator::get_pipeline(const rhi_compute_pipeline_desc& desc)
{
    return m_compute_pipeline_allocator.allocate(
        desc,
        [](const rhi_compute_pipeline_desc& desc)
        {
            return render_device::instance().create_pipeline(desc);
        });
}

rhi_sampler* transient_allocator::get_sampler(const rhi_sampler_desc& desc)
{
    return m_sampler_allocator.allocate(
        desc,
        [](const rhi_sampler_desc& desc)
        {
            return render_device::instance().create_sampler(desc);
        });
}

void transient_allocator::tick()
{
    std::size_t frame = render_device::instance().get_frame_count();
    if (frame > 1000 && frame % 1000 == 0)
    {
        std::size_t gc_frame = frame - 1000;

        m_parameter_allocator.gc(gc_frame);
        m_texture_allocator.gc(gc_frame);
        m_buffer_allocator.gc(gc_frame);
        m_render_pass_allocator.gc(gc_frame);
        m_raster_pipeline_allocator.gc(gc_frame);
        m_compute_pipeline_allocator.gc(gc_frame);
        m_sampler_allocator.gc(gc_frame);
    }

    m_parameter_allocator.tick();
    m_texture_allocator.tick();
    m_buffer_allocator.tick();
}

void transient_allocator::cleanup_dependents(const void* resource)
{
    auto iter = m_cleanup_functions.find(resource);
    if (iter != m_cleanup_functions.end())
    {
        for (auto& cleanup_functor : iter->second)
        {
            cleanup_functor();
        }

        m_cleanup_functions.erase(iter);
    }
}
} // namespace violet