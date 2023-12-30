#pragma once

#include "graphics/render_graph/render_node.hpp"
#include <memory>
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

enum render_parameter_type
{
    RENDER_PIPELINE_PARAMETER_TYPE_NORMAL,
    RENDER_PIPELINE_PARAMETER_TYPE_MESH,
    RENDER_PIPELINE_PARAMETER_TYPE_MATERIAL,
    RENDER_PIPELINE_PARAMETER_TYPE_CAMERA
};

class render_pass;
class render_pipeline : public render_node
{
public:
    using vertex_attributes = std::vector<std::pair<std::string, rhi_resource_format>>;
    using parameter_layouts = std::vector<std::pair<rhi_parameter_layout*, render_parameter_type>>;

public:
    render_pipeline();
    virtual ~render_pipeline();

    void set_shader(std::string_view vertex, std::string_view fragment);

    void set_vertex_attributes(const vertex_attributes& vertex_attributes);
    const vertex_attributes& get_vertex_attributes() const noexcept;

    void set_parameter_layouts(const parameter_layouts& parameter_layouts);
    rhi_parameter_layout* get_parameter_layout(render_parameter_type type) const noexcept;

    void set_blend(const rhi_blend_desc& blend) noexcept;
    void set_depth_stencil(const rhi_depth_stencil_desc& depth_stencil) noexcept;
    void set_cull_mode(rhi_cull_mode cull_mode) noexcept;

    void set_samples(rhi_sample_count samples) noexcept;
    void set_primitive_topology(rhi_primitive_topology primitive_topology) noexcept;

    void set_render_pass(rhi_render_pass* render_pass, std::size_t subpass) noexcept;

    virtual bool compile(compile_context& context) override;
    virtual void execute(execute_context& context) override;

    void add_mesh(const render_mesh& mesh);

private:
    virtual void render(std::vector<render_mesh>& meshes, const execute_context& context) = 0;

    std::string m_vertex_shader;
    std::string m_fragment_shader;
    vertex_attributes m_vertex_attributes;
    parameter_layouts m_parameter_layouts;

    rhi_render_pipeline_desc m_desc;

    rhi_ptr<rhi_render_pipeline> m_interface;

    std::vector<render_mesh> m_meshes;
};
} // namespace violet