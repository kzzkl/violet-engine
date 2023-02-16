#pragma once

#include "graphics_interface.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace violet::graphics
{
class rhi_plugin;
class rhi
{
public:
    static void initialize(std::string_view plugin, const rhi_desc& desc);

    static renderer_interface& renderer();
    static resource_format back_buffer_format();

    static std::unique_ptr<pipeline_parameter_interface> make_pipeline_parameter(
        const pipeline_parameter_desc& desc);
    static std::unique_ptr<render_pipeline_interface> make_render_pipeline(
        const render_pipeline_desc& desc);
    static std::unique_ptr<compute_pipeline_interface> make_compute_pipeline(
        const compute_pipeline_desc& desc);

    template <typename Vertex>
    static std::unique_ptr<resource_interface> make_vertex_buffer(
        const Vertex* data,
        std::size_t size,
        vertex_buffer_flags flags = VERTEX_BUFFER_FLAG_NONE,
        bool dynamic = false,
        bool frame_resource = false)
    {
        vertex_buffer_desc desc = {
            .vertices = data,
            .vertex_size = sizeof(Vertex),
            .vertex_count = size,
            .flags = flags,
            .dynamic = dynamic,
            .frame_resource = frame_resource};
        return std::unique_ptr<resource_interface>(impl().make_vertex_buffer(desc));
    }

    template <typename Index>
    static std::unique_ptr<resource_interface> make_index_buffer(
        const Index* data,
        std::size_t size,
        bool dynamic = false,
        bool frame_resource = false)
    {
        index_buffer_desc desc = {
            .indices = data,
            .index_size = sizeof(Index),
            .index_count = size,
            .dynamic = dynamic,
            .frame_resource = frame_resource};
        return std::unique_ptr<resource_interface>(impl().make_index_buffer(desc));
    }

    static std::unique_ptr<resource_interface> make_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height,
        resource_format format = RESOURCE_FORMAT_B8G8R8A8_UNORM);
    static std::unique_ptr<resource_interface> make_texture(std::string_view file);

    static std::unique_ptr<resource_interface> make_texture_cube(
        std::string_view right,
        std::string_view left,
        std::string_view top,
        std::string_view bottom,
        std::string_view front,
        std::string_view back);

    static std::unique_ptr<resource_interface> make_shadow_map(const shadow_map_desc& desc);

    static std::unique_ptr<resource_interface> make_render_target(const render_target_desc& desc);
    static std::unique_ptr<resource_interface> make_depth_stencil_buffer(
        const depth_stencil_buffer_desc& desc);

private:
    template <typename T>
    using interface_map = std::unordered_map<std::string, std::unique_ptr<T>>;

    static rhi& instance();
    static rhi_interface& impl();

    std::unique_ptr<rhi_plugin> m_plugin;

    std::unique_ptr<renderer_interface> m_renderer;
};
} // namespace violet::graphics