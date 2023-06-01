#pragma once

#include "graphics/render_graph/render_node.hpp"
#include "graphics/render_graph/render_pass.hpp"
#include "graphics/rhi.hpp"
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
    render_pipeline(std::string_view name, std::size_t index);
    virtual ~render_pipeline();

    void set_shader(std::string_view vertex, std::string_view pixel);
    void set_vertex_layout(const std::vector<vertex_attribute>& vertex_layout);
    void set_parameter_layout(const std::vector<pipeline_parameter_desc>& parameter_layout);

    void set_blend(const blend_desc& blend) noexcept;
    void set_depth_stencil(const depth_stencil_desc& depth_stencil) noexcept;
    void set_rasterizer(const rasterizer_desc& rasterizer) noexcept;

    void set_samples(std::size_t samples) noexcept;
    void set_primitive_topology(primitive_topology primitive_topology) noexcept;

    void set_render_pass(render_pass* render_pass, std::size_t subpass) noexcept;

    virtual bool compile() override;

private:
    std::unique_ptr<rhi_render_pipeline> m_interface;

    std::string m_vertex_shader;
    std::string m_pixel_shader;
    std::vector<vertex_attribute> m_vertex_layout;
    std::vector<pipeline_parameter_desc> m_parameter_layout;

    blend_desc m_blend;
    depth_stencil_desc m_depth_stencil;
    rasterizer_desc m_rasterizer;

    std::size_t m_samples;
    primitive_topology m_primitive_topology;

    render_pass* m_render_pass;
    std::size_t m_subpass;
};
} // namespace violet