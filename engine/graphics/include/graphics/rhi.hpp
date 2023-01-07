#pragma once

#include "graphics_interface.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace violet::graphics
{
using attachment_info = attachment_desc;

struct render_pass_info
{
    render_pass_info();

    std::vector<vertex_attribute> vertex_attributes;
    std::vector<std::string> parameters;
    std::vector<attachment_reference> references;

    std::string vertex_shader;
    std::string pixel_shader;

    blend_desc blend;
    depth_stencil_desc depth_stencil;
    rasterizer_desc rasterizer;

    primitive_topology_type primitive_topology;
    std::size_t samples;
};

struct render_pipeline_info
{
    std::vector<attachment_info> attachments;
    std::vector<render_pass_info> passes;
};

struct compute_pipeline_info
{
    std::string compute_shader;
    std::vector<std::string> parameters;
};

using shadow_map_info = shadow_map_desc;
using depth_stencil_buffer_info = depth_stencil_buffer_desc;
using render_target_info = render_target_desc;

using rhi_info = rhi_desc;

class rhi_plugin;
class rhi
{
public:
    static void initialize(std::string_view plugin, const rhi_info& info);

    static renderer_interface& renderer();
    static resource_format back_buffer_format();

    static pipeline_parameter_layout_interface* register_pipeline_parameter_layout(
        std::string_view name,
        const std::vector<pipeline_parameter_pair>& parameters);
    static pipeline_parameter_layout_interface* find_pipeline_parameter_layout(
        std::string_view name);
    static std::unique_ptr<pipeline_parameter_interface> make_pipeline_parameter(
        pipeline_parameter_layout_interface* layout);

    static std::unique_ptr<render_pipeline_interface> make_render_pipeline(
        const render_pipeline_info& info);
    static std::unique_ptr<compute_pipeline_interface> make_compute_pipeline(
        const compute_pipeline_info& info);

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

    static std::unique_ptr<resource_interface> make_shadow_map(const shadow_map_info& info);

    static std::unique_ptr<resource_interface> make_render_target(const render_target_info& info);
    static std::unique_ptr<resource_interface> make_depth_stencil_buffer(
        const depth_stencil_buffer_info& info);

private:
    template <typename T>
    using interface_map = std::unordered_map<std::string, std::unique_ptr<T>>;

    static rhi& instance();
    static rhi_interface& impl();

    std::unique_ptr<rhi_plugin> m_plugin;

    std::unique_ptr<renderer_interface> m_renderer;
    interface_map<pipeline_parameter_layout_interface> m_parameter_layouts;
};
} // namespace violet::graphics