#include "graphics/render_graph/rdg_allocator.hpp"

namespace violet
{
rdg_allocator::rdg_allocator() {}

rhi_parameter* rdg_allocator::allocate_parameter(const rhi_parameter_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_parameter_desc));

    auto& pool = m_parameter_pools[hash];
    if (pool.count == pool.data.size())
    {
        rhi_parameter_desc copy = desc;
        copy.flags = RHI_PARAMETER_DISABLE_SYNC;
        pool.data.push_back(render_device::instance().create_parameter(copy));
    }

    return pool.data[pool.count++].get();
}

rhi_texture* rdg_allocator::allocate_texture(const rhi_texture_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_texture_desc));

    auto& pool = m_texture_pools[hash];
    if (pool.count == pool.data.size())
    {
        pool.data.push_back(render_device::instance().create_texture(desc));
    }

    return pool.data[pool.count++].get();
}

rhi_buffer* rdg_allocator::allocate_buffer(const rhi_buffer_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_buffer_desc));

    auto& pool = m_buffer_pools[hash];
    if (pool.count == pool.data.size())
    {
        pool.data.push_back(render_device::instance().create_buffer(desc));
    }

    return pool.data[pool.count++].get();
}

rhi_render_pass* rdg_allocator::get_render_pass(const rhi_render_pass_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_render_pass_desc));

    rhi_render_pass* render_pass = m_render_pass_cache.get(hash);
    if (render_pass == nullptr)
    {
        return m_render_pass_cache.add(hash, render_device::instance().create_render_pass(desc));
    }
    else
    {
        return render_pass;
    }
}

rhi_framebuffer* rdg_allocator::get_framebuffer(const rhi_framebuffer_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_framebuffer_desc));

    rhi_framebuffer* framebuffer = m_framebuffer_cache.get(hash);
    if (framebuffer == nullptr)
    {
        return m_framebuffer_cache.add(hash, render_device::instance().create_framebuffer(desc));
    }
    else
    {
        return framebuffer;
    }
}

rhi_render_pipeline* rdg_allocator::get_pipeline(
    const rdg_render_pipeline& pipeline,
    rhi_render_pass* render_pass,
    std::uint32_t subpass_index)
{
    std::uint64_t hash = hash::city_hash_64(&pipeline, sizeof(rdg_render_pipeline));
    hash = hash::combine(hash, std::hash<rhi_render_pass*>()(render_pass));
    hash = hash::combine(hash, std::hash<std::uint32_t>()(subpass_index));

    rhi_render_pipeline* render_pipeline = m_render_pipeline_cache.get(hash);
    if (render_pipeline == nullptr)
    {
        rhi_render_pipeline_desc desc = {
            .vertex_shader = pipeline.vertex_shader,
            .fragment_shader = pipeline.fragment_shader,
            .blend = pipeline.blend,
            .depth_stencil = pipeline.depth_stencil,
            .rasterizer = pipeline.rasterizer,
            .samples = pipeline.samples,
            .primitive_topology = pipeline.primitive_topology,
            .render_pass = render_pass,
            .subpass_index = subpass_index,
        };

        return m_render_pipeline_cache.add(hash, render_device::instance().create_pipeline(desc));
    }
    else
    {
        return render_pipeline;
    }
}

rhi_compute_pipeline* rdg_allocator::get_pipeline(const rdg_compute_pipeline& pipeline)
{
    std::uint64_t hash = hash::city_hash_64(&pipeline, sizeof(rdg_compute_pipeline));

    rhi_compute_pipeline* compute_pipeline = m_compute_pipeline_cache.get(hash);
    if (compute_pipeline == nullptr)
    {
        rhi_compute_pipeline_desc desc = {.compute_shader = pipeline.compute_shader};
        return m_compute_pipeline_cache.add(hash, render_device::instance().create_pipeline(desc));
    }
    else
    {
        return compute_pipeline;
    }
}

rhi_sampler* rdg_allocator::get_sampler(const rhi_sampler_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_sampler_desc));

    rhi_sampler* sampler = m_sampler_cache.get(hash);
    if (sampler == nullptr)
        return m_sampler_cache.add(hash, render_device::instance().create_sampler(desc));
    else
        return sampler;
}

void rdg_allocator::reset()
{
    for (auto& [hash, pool] : m_parameter_pools)
    {
        pool.count = 0;
    }

    for (auto& [hash, pool] : m_texture_pools)
    {
        pool.count = 0;
    }

    for (auto& [hash, pool] : m_buffer_pools)
    {
        pool.count = 0;
    }

    gc();
}

void rdg_allocator::gc()
{
    std::size_t frame = render_device::instance().get_frame_count();
    if (frame < 1000 || frame % 1000 != 0)
        return;

    std::size_t gc_frame = frame - 1000;

    m_render_pass_cache.gc(gc_frame);
    m_framebuffer_cache.gc(gc_frame);
    // m_render_pipeline_cache.gc(gc_frame);
    // m_compute_pipeline_cache.gc(gc_frame);
}
} // namespace violet