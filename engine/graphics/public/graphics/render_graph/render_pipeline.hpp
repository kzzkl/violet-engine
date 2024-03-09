#pragma once

#include "graphics/renderer.hpp"
#include <memory>
#include <string>
#include <vector>

namespace violet
{
struct render_mesh
{
    std::vector<rhi_buffer*> vertex_buffers;
    rhi_buffer* index_buffer;

    std::size_t vertex_start;
    std::size_t vertex_count;
    std::size_t index_start;
    std::size_t index_count;

    rhi_parameter* transform;
    rhi_parameter* material;
};

class render_pass;
class render_pipeline
{
public:
    using vertex_attributes = std::vector<std::pair<std::string, rhi_resource_format>>;

public:
    render_pipeline(
        render_pass* render_pass,
        std::size_t subpass_index,
        std::size_t color_attachment_count);
    ~render_pipeline();

    void set_shader(std::string_view vertex, std::string_view fragment);

    void set_vertex_attributes(const vertex_attributes& vertex_attributes);
    const vertex_attributes& get_vertex_attributes() const noexcept;

    void set_parameter_layouts(const std::vector<rhi_parameter_layout*>& parameter_layouts);

    void set_material_parameter_layout(rhi_parameter_layout* layout) noexcept
    {
        m_material_layout = layout;
    }
    rhi_parameter_layout* get_material_parameter_layout() const noexcept
    {
        return m_material_layout;
    }

    void set_blend(
        std::size_t attachment_index,
        const rhi_attachment_blend_desc& attachment_blend) noexcept;
    void set_depth_stencil(const rhi_depth_stencil_desc& depth_stencil) noexcept;
    void set_cull_mode(rhi_cull_mode cull_mode) noexcept;

    void set_samples(rhi_sample_count samples) noexcept;
    void set_primitive_topology(rhi_primitive_topology primitive_topology) noexcept;

    bool compile(renderer* renderer);

    void add_mesh(const render_mesh& mesh);
    const std::vector<render_mesh>& get_meshes() const noexcept;
    void clear_mesh();

    rhi_render_pipeline* get_interface() const noexcept { return m_interface.get(); }

private:
    std::string m_vertex_shader;
    std::string m_fragment_shader;
    vertex_attributes m_vertex_attributes;
    std::vector<rhi_parameter_layout*> m_parameter_layouts;

    render_pass* m_render_pass;
    rhi_render_pipeline_desc m_desc;

    rhi_parameter_layout* m_material_layout;
    rhi_ptr<rhi_render_pipeline> m_interface;

    std::vector<render_mesh> m_meshes;
};

} // namespace violet