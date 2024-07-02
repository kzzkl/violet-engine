#pragma once

#include "common/type_index.hpp"
#include "graphics/render_device.hpp"
#include "graphics/render_graph/rdg_pipeline.hpp"

namespace violet
{
class rdg_data
{
public:
    virtual ~rdg_data() = default;

    virtual void reset() = 0;

private:
};

class rdg_allocator
{
private:
    struct data_index : public type_index<data_index, std::size_t>
    {
    };

public:
    rdg_allocator();

    template <typename T>
    T& allocate_data()
    {
        if (m_data_pools.size() <= data_index::value<T>())
            m_data_pools.resize(data_index::value<T>() + 1);

        auto& pool = m_data_pools[data_index::value<T>()];

        if (pool.count == pool.data.size())
            pool.data.push_back(std::make_unique<T>());

        return *static_cast<T*>(pool.data[pool.count++].get());
    }

    rhi_command* allocate_command() { return render_device::instance().allocate_command(); }

    rhi_render_pass* get_render_pass(const rhi_render_pass_desc& desc);
    rhi_framebuffer* get_framebuffer(const rhi_framebuffer_desc& desc);

    rhi_render_pipeline* get_pipeline(
        const rdg_render_pipeline& pipeline,
        rhi_render_pass* render_pass,
        std::uint32_t subpass_index);

    void reset();

private:
    struct data_pool
    {
        std::size_t count;
        std::vector<std::unique_ptr<rdg_data>> data;
    };

    std::unordered_map<std::uint64_t, rhi_ptr<rhi_render_pass>> m_render_pass_cache;
    std::unordered_map<std::uint64_t, rhi_ptr<rhi_framebuffer>> m_framebuffer_cache;
    std::unordered_map<std::uint64_t, rhi_ptr<rhi_render_pipeline>> m_render_pipeline_cache;
    std::unordered_map<std::uint64_t, rhi_ptr<rhi_compute_pipeline>> m_compute_pipeline_cache;

    std::vector<data_pool> m_data_pools;
};
} // namespace violet