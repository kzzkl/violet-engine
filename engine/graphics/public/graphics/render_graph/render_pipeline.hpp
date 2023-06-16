#pragma once

#include "graphics/render_graph/render_node.hpp"
#include <memory>
#include <string>
#include <vector>

namespace violet
{
struct render_item
{
    rhi_resource* index_buffer;
    rhi_resource* const* vertex_buffers;

    rhi_pipeline_parameter* node_parameter;
    rhi_pipeline_parameter* material_parameter;
};

class render_pass;
class render_pipeline : public render_node
{
public:
    using vertex_layout = std::vector<std::pair<std::string, rhi_resource_format>>;

public:
    render_pipeline(
        std::string_view name,
        rhi_context* rhi,
        render_pass* render_pass,
        std::size_t subpass);
    virtual ~render_pipeline();

    void set_shader(std::string_view vertex, std::string_view pixel);

    void set_vertex_layout(const vertex_layout& vertex_layout);
    const vertex_layout& get_vertex_layout() const noexcept;

    void set_parameter_layout(const std::vector<rhi_pipeline_parameter_desc>& parameter_layout);

    void set_blend(const rhi_blend_desc& blend) noexcept;
    void set_depth_stencil(const rhi_depth_stencil_desc& depth_stencil) noexcept;
    void set_rasterizer(const rhi_rasterizer_desc& rasterizer) noexcept;

    void set_samples(std::size_t samples) noexcept;
    void set_primitive_topology(rhi_primitive_topology primitive_topology) noexcept;

    render_pass* get_render_pass() const noexcept { return m_render_pass; }
    std::size_t get_subpass() const noexcept { return m_subpass; }

    virtual bool compile() override;

    rhi_render_pipeline* get_interface() const noexcept { return m_interface; }

private:
    std::string m_vertex_shader;
    std::string m_pixel_shader;
    vertex_layout m_vertex_layout;
    std::vector<rhi_pipeline_parameter_desc> m_parameter_layout;

    rhi_blend_desc m_blend;
    rhi_depth_stencil_desc m_depth_stencil;
    rhi_rasterizer_desc m_rasterizer;

    std::size_t m_samples;
    rhi_primitive_topology m_primitive_topology;

    render_pass* m_render_pass;
    std::size_t m_subpass;

    rhi_render_pipeline* m_interface;
};
} // namespace violet