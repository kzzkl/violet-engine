#pragma once

#include "d3d12_pipeline.hpp"

namespace violet::graphics::d3d12
{
namespace
{
template <class T>
void hash_combine(std::size_t& s, const T& v)
{
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

template <typename T>
struct d3d12_cache_trait
{
    using value_type = T;
};

template <>
struct d3d12_cache_trait<d3d12_frame_buffer>
{
    using key_type = d3d12_camera_info;
    using value_type = d3d12_frame_buffer;

    struct hash
    {
        std::size_t operator()(const d3d12_camera_info& value) const
        {
            std::size_t result = 0;
            hash_combine(result, value.render_target);
            hash_combine(result, value.render_target_resolve);
            hash_combine(result, value.depth_stencil_buffer);

            return result;
        }
    };

    struct equal
    {
        bool operator()(const d3d12_camera_info& a, const d3d12_camera_info& b) const
        {
            return a.render_target == b.render_target &&
                   a.render_target_resolve == b.render_target_resolve &&
                   a.depth_stencil_buffer == b.depth_stencil_buffer;
        }
    };
};

template <>
struct d3d12_cache_trait<d3d12_pipeline_parameter_layout>
{
    using key_type = pipeline_parameter_desc;
    using value_type = d3d12_pipeline_parameter_layout;

    struct hash
    {
        std::size_t operator()(const pipeline_parameter_desc& value) const
        {
            std::size_t result = 0;
            for (std::size_t i = 0; i < value.parameter_count; ++i)
            {
                hash_combine(result, value.parameters[i].type);
                hash_combine(result, value.parameters[i].size);
            }

            return result;
        }
    };

    struct equal
    {
        bool operator()(const pipeline_parameter_desc& a, const pipeline_parameter_desc& b) const
        {
            if (a.parameter_count != b.parameter_count)
                return false;

            for (std::size_t i = 0; i < a.parameter_count; ++i)
            {
                if (a.parameters[i].type != b.parameters[i].type ||
                    a.parameters[i].size != a.parameters[i].size)
                    return false;
            }

            return true;
        }
    };
};
} // namespace

class d3d12_cache
{
public:
    d3d12_frame_buffer* get_or_create_frame_buffer(
        d3d12_render_pipeline* pipeline,
        const d3d12_camera_info& camera_info);

    d3d12_pipeline_parameter_layout* get_or_create_pipeline_parameter_layout(
        const pipeline_parameter_desc& desc);

    void on_resource_destroy(d3d12_resource* resource);

private:
    template <typename T>
    using cache_map = std::unordered_map<
        typename d3d12_cache_trait<T>::key_type,
        std::unique_ptr<typename d3d12_cache_trait<T>::value_type>,
        typename d3d12_cache_trait<T>::hash,
        typename d3d12_cache_trait<T>::equal>;

    cache_map<d3d12_frame_buffer> m_frame_buffers;
    cache_map<d3d12_pipeline_parameter_layout> m_pipeline_parameter_layouts;
};
} // namespace violet::graphics::d3d12