#include "graphics/render_graph/rdg_allocator.hpp"

namespace violet
{
rdg_allocator::rdg_allocator()
{
}

rhi_parameter* rdg_allocator::allocate_parameter(const rhi_parameter_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_parameter_desc));

    auto& pool = m_parameter_pools[hash];
    if (pool.count == pool.data.size())
        pool.data.push_back(render_device::instance().create_parameter(desc));

    return pool.data[pool.count++].get();
}

rhi_texture* rdg_allocator::allocate_texture(const rhi_texture_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_texture_desc));

    auto& pool = m_texture_pools[hash];
    if (pool.count == pool.data.size())
        pool.data.push_back(render_device::instance().create_texture(desc));

    return pool.data[pool.count++].get();
}

rhi_texture* rdg_allocator::allocate_texture(const rhi_texture_view_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_texture_view_desc));

    auto& pool = m_texture_pools[hash];
    if (pool.count == pool.data.size())
        pool.data.push_back(render_device::instance().create_texture_view(desc));

    return pool.data[pool.count++].get();
}

rhi_render_pass* rdg_allocator::get_render_pass(const rhi_render_pass_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_render_pass_desc));

    rhi_render_pass* render_pass = m_render_pass_cache.get(hash);
    if (render_pass == nullptr)
        return m_render_pass_cache.add(hash, render_device::instance().create_render_pass(desc));
    else
        return render_pass;
}

rhi_framebuffer* rdg_allocator::get_framebuffer(const rhi_framebuffer_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_framebuffer_desc));

    rhi_framebuffer* framebuffer = m_framebuffer_cache.get(hash);
    if (framebuffer == nullptr)
        return m_framebuffer_cache.add(hash, render_device::instance().create_framebuffer(desc));
    else
        return framebuffer;
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
        rhi_render_pipeline_desc desc = {};
        desc.vertex_shader = pipeline.vertex_shader;
        desc.fragment_shader = pipeline.fragment_shader;
        desc.blend = pipeline.blend;
        desc.depth_stencil = pipeline.depth_stencil;
        desc.rasterizer = pipeline.rasterizer;
        desc.samples = pipeline.samples;
        desc.primitive_topology = pipeline.primitive_topology;
        desc.render_pass = render_pass;
        desc.render_subpass_index = subpass_index;

        return m_render_pipeline_cache.add(hash, render_device::instance().create_pipeline(desc));
    }
    else
    {
        return render_pipeline;
    }
}

rhi_sampler* rdg_allocator::get_sampler(const rhi_sampler_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_parameter_desc));

    rhi_sampler* sampler = m_sampler_cache.get(hash);
    if (sampler == nullptr)
        return m_sampler_cache.add(hash, render_device::instance().create_sampler(desc));
    else
        return sampler;
}

void rdg_allocator::reset()
{
    for (auto& pool : m_data_pools)
        pool.count = 0;

    for (auto& [hash, pool] : m_parameter_pools)
        pool.count = 0;

    for (auto& [hash, pool] : m_texture_pools)
        pool.count = 0;

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