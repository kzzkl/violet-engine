#include "graphics/render_graph/rdg_allocator.hpp"

namespace violet
{
rdg_allocator::rdg_allocator()
{
}

rhi_render_pass* rdg_allocator::get_render_pass(const rhi_render_pass_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_render_pass_desc));

    auto iter = m_render_pass_cache.find(hash);
    if (iter == m_render_pass_cache.end())
    {
        m_render_pass_cache[hash] = render_device::instance().create_render_pass(desc);
        return m_render_pass_cache[hash].get();
    }
    else
    {
        return iter->second.get();
    }
}

rhi_framebuffer* rdg_allocator::get_framebuffer(const rhi_framebuffer_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_framebuffer_desc));

    auto iter = m_framebuffer_cache.find(hash);
    if (iter == m_framebuffer_cache.end())
    {
        m_framebuffer_cache[hash] = render_device::instance().create_framebuffer(desc);
        return m_framebuffer_cache[hash].get();
    }
    else
    {
        return iter->second.get();
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

    auto iter = m_render_pipeline_cache.find(hash);
    if (iter == m_render_pipeline_cache.end())
    {
        rhi_render_pipeline_desc desc = {};
        desc.vertex_shader = pipeline.vertex_shader;
        desc.fragment_shader = pipeline.fragment_shader;
        desc.input = pipeline.input;
        desc.blend = pipeline.blend;
        desc.depth_stencil = pipeline.depth_stencil;
        desc.rasterizer = pipeline.rasterizer;
        desc.samples = pipeline.samples;
        desc.primitive_topology = pipeline.primitive_topology;
        desc.render_pass = render_pass;
        desc.render_subpass_index = subpass_index;

        std::memcpy(
            desc.parameters,
            pipeline.parameters,
            pipeline.parameter_count * sizeof(rhi_parameter_desc));
        desc.parameter_count = pipeline.parameter_count;

        m_render_pipeline_cache[hash] = render_device::instance().create_pipeline(desc);
        return m_render_pipeline_cache[hash].get();
    }
    else
    {
        return iter->second.get();
    }
}

void rdg_allocator::reset()
{
    for (auto& pool : m_data_pools)
    {
        for (auto& data : pool.data)
            data->reset();
        pool.count = 0;
    }
}
} // namespace violet