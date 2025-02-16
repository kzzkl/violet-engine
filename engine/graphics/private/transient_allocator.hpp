#pragma once

#include "graphics/render_device.hpp"
#include <functional>

namespace std
{
template <>
struct hash<violet::rhi_parameter_desc>
{
    std::size_t operator()(const violet::rhi_parameter_desc& desc) const noexcept
    {
        return violet::hash::city_hash_64(
            desc.bindings,
            sizeof(violet::rhi_parameter_binding) * desc.binding_count);
    }
};

template <>
struct hash<violet::rhi_texture_desc>
{
    std::size_t operator()(const violet::rhi_texture_desc& desc) const noexcept
    {
        return violet::hash::city_hash_64(&desc, sizeof(violet::rhi_texture_desc));
    }
};

template <>
struct hash<violet::rhi_buffer_desc>
{
    std::size_t operator()(const violet::rhi_buffer_desc& desc) const noexcept
    {
        return violet::hash::city_hash_64(&desc, sizeof(violet::rhi_buffer_desc));
    }
};

template <>
struct hash<violet::rhi_render_pass_desc>
{
    std::size_t operator()(const violet::rhi_render_pass_desc& desc) const noexcept
    {
        return violet::hash::city_hash_64(&desc, sizeof(violet::rhi_render_pass_desc));
    }
};

template <>
struct hash<violet::rhi_raster_pipeline_desc>
{
    std::size_t operator()(const violet::rhi_raster_pipeline_desc& desc) const noexcept
    {
        return violet::hash::city_hash_64(&desc, sizeof(violet::rhi_raster_pipeline_desc));
    }
};

template <>
struct hash<violet::rhi_compute_pipeline_desc>
{
    std::size_t operator()(const violet::rhi_compute_pipeline_desc& desc) const noexcept
    {
        return violet::hash::city_hash_64(&desc, sizeof(violet::rhi_compute_pipeline_desc));
    }
};

template <>
struct hash<violet::rhi_sampler_desc>
{
    std::size_t operator()(const violet::rhi_sampler_desc& desc) const noexcept
    {
        return violet::hash::city_hash_64(&desc, sizeof(violet::rhi_sampler_desc));
    }
};
} // namespace std

namespace violet
{
inline bool operator==(const rhi_texture_extent& a, const rhi_texture_extent& b) noexcept
{
    return a.width == b.width && a.height == b.height;
}

inline bool operator==(const rhi_parameter_desc& a, const rhi_parameter_desc& b) noexcept
{
    return a.bindings == b.bindings && a.binding_count == b.binding_count && a.flags == b.flags;
}

inline bool operator==(const rhi_texture_desc& a, const rhi_texture_desc& b) noexcept
{
    return a.extent == b.extent && a.format == b.format && a.flags == b.flags &&
           a.level_count == b.level_count && a.layer_count == b.layer_count &&
           a.samples == b.samples && a.layout == b.layout;
}

inline bool operator==(const rhi_buffer_desc& a, const rhi_buffer_desc& b) noexcept
{
    return a.data == b.data && a.size == b.size && a.flags == b.flags;
}

inline bool operator==(const rhi_attachment_desc& a, const rhi_attachment_desc& b) noexcept
{
    return a.format == b.format && a.samples == b.samples && a.layout == b.layout &&
           a.initial_layout == b.initial_layout && a.final_layout == b.final_layout &&
           a.load_op == b.load_op && a.store_op == b.store_op &&
           a.stencil_load_op == b.stencil_load_op && a.stencil_store_op == b.stencil_store_op;
}

inline bool operator==(
    const rhi_render_pass_dependency_desc& a,
    const rhi_render_pass_dependency_desc& b) noexcept
{
    return a.src_stages == b.src_stages && a.src_access == b.src_access &&
           a.dst_stages == b.dst_stages && a.dst_access == b.dst_access;
}

inline bool operator==(const rhi_render_pass_desc& a, const rhi_render_pass_desc& b) noexcept
{
    if (a.attachment_count != b.attachment_count)
    {
        return false;
    }

    for (std::size_t i = 0; i < a.attachment_count; ++i)
    {
        if (a.attachments[i] != b.attachments[i])
        {
            return false;
        }
    }

    return a.begin_dependency == b.begin_dependency && a.end_dependency == b.end_dependency;
}

inline bool operator==(
    const rhi_raster_pipeline_desc& a,
    const rhi_raster_pipeline_desc& b) noexcept
{
    return a.vertex_shader == b.vertex_shader && a.fragment_shader == b.fragment_shader &&
           a.render_pass == b.render_pass;
}

inline bool operator==(
    const rhi_compute_pipeline_desc& a,
    const rhi_compute_pipeline_desc& b) noexcept
{
    return a.compute_shader == b.compute_shader;
}

inline bool operator==(const rhi_sampler_desc& a, const rhi_sampler_desc& b) noexcept
{
    return a.mag_filter == b.mag_filter && a.min_filter == b.min_filter &&
           a.address_mode_u == b.address_mode_u && a.address_mode_v == b.address_mode_v &&
           a.address_mode_w == b.address_mode_w && a.min_level == b.min_level &&
           a.max_level == b.max_level;
}

template <typename Functor, typename Desc, typename Object>
concept create_functor = requires(Functor functor, const Desc& desc) {
    { functor(desc) } -> std::same_as<rhi_ptr<Object>>;
};

template <typename T, typename Desc>
class shared_allocator
{
public:
    using key_type = Desc;
    using object_type = T;

    template <typename CreateFunctor>
        requires create_functor<CreateFunctor, Desc, T>
    T* allocate(const key_type& desc, CreateFunctor&& create)
    {
        auto iter = m_objects.find(desc);
        if (iter != m_objects.end())
        {
            iter->second.last_used_frame = render_device::instance().get_frame_count();
            return iter->second.object.get();
        }

        m_objects[desc] = {
            .object = create(desc),
            .last_used_frame = render_device::instance().get_frame_count(),
        };

        return m_objects[desc].object.get();
    }

    void gc(std::size_t frame)
    {
        for (auto iter = m_objects.begin(); iter != m_objects.end();)
        {
            if (iter->second.last_used_frame < frame)
            {
                iter = m_objects.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    void remove(const key_type& desc)
    {
        auto iter = m_objects.find(desc);
        if (iter != m_objects.end())
        {
            m_objects.erase(desc);
        }
    }

private:
    struct wrapper
    {
        rhi_ptr<object_type> object;
        std::size_t last_used_frame;
    };

    std::unordered_map<key_type, wrapper> m_objects;
};

template <typename T, typename Desc>
class unique_allocator
{
public:
    using key_type = Desc;
    using object_type = T;

    template <typename CreateFunctor>
    object_type* allocate(const key_type& desc, CreateFunctor&& create)
    {
        auto& pool = m_pools[desc];

        if (pool.count == pool.objects.size())
        {
            pool.objects.push_back({
                .object = create(desc),
                .last_used_frame = render_device::instance().get_frame_count(),
            });
        }

        auto& result = pool.objects[pool.count++];
        result.last_used_frame = render_device::instance().get_frame_count();
        return result.object.get();
    }

    void tick()
    {
        for (auto& [desc, pool] : m_pools)
        {
            pool.count = 0;
        }
    }

    void gc(std::size_t frame)
    {
        for (auto iter = m_pools.begin(); iter != m_pools.end();)
        {
            iter->second.objects.erase(
                std::remove_if(
                    iter->second.objects.begin(),
                    iter->second.objects.end(),
                    [frame](const auto& object)
                    {
                        return object.last_used_frame < frame;
                    }),
                iter->second.objects.end());

            if (iter->second.objects.empty())
            {
                iter = m_pools.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

private:
    struct wrapper
    {
        rhi_ptr<object_type> object;
        std::size_t last_used_frame;
    };

    struct object_pool
    {
        std::size_t count;
        std::vector<wrapper> objects;
    };

    std::unordered_map<key_type, object_pool> m_pools;
};

class transient_allocator
{
public:
    rhi_parameter* allocate_parameter(const rhi_parameter_desc& desc);

    rhi_texture* allocate_texture(const rhi_texture_desc& desc);
    rhi_buffer* allocate_buffer(const rhi_buffer_desc& desc);

    rhi_render_pass* get_render_pass(const rhi_render_pass_desc& desc);

    rhi_raster_pipeline* get_pipeline(const rhi_raster_pipeline_desc& desc);
    rhi_compute_pipeline* get_pipeline(const rhi_compute_pipeline_desc& desc);

    rhi_sampler* get_sampler(const rhi_sampler_desc& desc);

    void tick();

    void cleanup_dependents(const void* resource);

private:
    unique_allocator<rhi_parameter, rhi_parameter_desc> m_parameter_allocator;
    unique_allocator<rhi_texture, rhi_texture_desc> m_texture_allocator;
    unique_allocator<rhi_buffer, rhi_buffer_desc> m_buffer_allocator;

    shared_allocator<rhi_render_pass, rhi_render_pass_desc> m_render_pass_allocator;
    shared_allocator<rhi_raster_pipeline, rhi_raster_pipeline_desc> m_raster_pipeline_allocator;
    shared_allocator<rhi_compute_pipeline, rhi_compute_pipeline_desc> m_compute_pipeline_allocator;
    shared_allocator<rhi_sampler, rhi_sampler_desc> m_sampler_allocator;

    std::unordered_map<const void*, std::vector<std::function<void()>>> m_cleanup_functions;
};
} // namespace violet