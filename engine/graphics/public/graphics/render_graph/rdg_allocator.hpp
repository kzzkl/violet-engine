#pragma once

#include "common/type_index.hpp"
#include "graphics/render_device.hpp"
#include "graphics/render_graph/rdg_pipeline.hpp"
#include <cassert>

namespace violet
{
class rdg_allocator
{
private:
    struct data_index : public type_index<data_index, std::size_t>
    {
    };

public:
    rdg_allocator();

    rhi_parameter* allocate_parameter(const rhi_parameter_desc& desc);

    rhi_texture* allocate_texture(const rhi_texture_desc& desc);

    rhi_buffer* allocate_buffer(const rhi_buffer_desc& desc);

    rhi_command* allocate_command()
    {
        return render_device::instance().allocate_command();
    }

    rhi_render_pass* get_render_pass(const rhi_render_pass_desc& desc);
    rhi_framebuffer* get_framebuffer(const rhi_framebuffer_desc& desc);

    rhi_render_pipeline* get_pipeline(
        const rdg_render_pipeline& pipeline,
        rhi_render_pass* render_pass,
        std::uint32_t subpass_index);
    rhi_compute_pipeline* get_pipeline(const rdg_compute_pipeline& pipeline);

    rhi_sampler* get_sampler(const rhi_sampler_desc& desc);

    void reset();

private:
    template <typename T>
    class rhi_cache
    {
    public:
        using object_type = T;

    public:
        object_type* add(std::uint64_t hash, rhi_ptr<object_type>&& object)
        {
            assert(m_objects.find(hash) == m_objects.end());

            object_type* result = object.get();

            wrapper wrapper = {
                .last_used_frame = render_device::instance().get_frame_count(),
                .object = std::move(object)};
            m_objects[hash] = std::move(wrapper);

            return result;
        }

        object_type* get(std::uint64_t hash)
        {
            auto iter = m_objects.find(hash);
            if (iter != m_objects.end())
            {
                iter->second.last_used_frame = render_device::instance().get_frame_count();
                return iter->second.object.get();
            }
            else
            {
                return nullptr;
            }
        }

        void gc(std::size_t frame)
        {
            for (auto iter = m_objects.begin(); iter != m_objects.end();)
            {
                if (iter->second.last_used_frame < frame)
                    iter = m_objects.erase(iter);
                else
                    ++iter;
            }
        }

    private:
        struct wrapper
        {
            std::size_t last_used_frame;
            rhi_ptr<object_type> object;
        };

        std::unordered_map<std::uint64_t, wrapper> m_objects;
    };

    template <typename T>
    struct resource_pool
    {
        std::size_t count;
        std::vector<rhi_ptr<T>> data;
    };

    void gc();

    rhi_cache<rhi_render_pass> m_render_pass_cache;
    rhi_cache<rhi_framebuffer> m_framebuffer_cache;
    rhi_cache<rhi_render_pipeline> m_render_pipeline_cache;
    rhi_cache<rhi_compute_pipeline> m_compute_pipeline_cache;
    rhi_cache<rhi_sampler> m_sampler_cache;

    std::unordered_map<std::uint64_t, resource_pool<rhi_parameter>> m_parameter_pools;
    std::unordered_map<std::uint64_t, resource_pool<rhi_texture>> m_texture_pools;
    std::unordered_map<std::uint64_t, resource_pool<rhi_buffer>> m_buffer_pools;
};
} // namespace violet