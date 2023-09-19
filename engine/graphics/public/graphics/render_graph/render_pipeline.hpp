#pragma once

#include "graphics/render_graph/render_node.hpp"
#include <memory>
#include <string>
#include <vector>

namespace violet
{
struct render_mesh
{
    std::vector<rhi_resource*> vertex_buffers;
    rhi_resource* index_buffer;

    std::size_t vertex_base;
    std::size_t index_start;
    std::size_t index_count;

    rhi_pipeline_parameter* node;
    rhi_pipeline_parameter* material;
};

struct render_context
{
    rhi_pipeline_parameter* camera_parameter;
    std::vector<render_mesh> meshes;
};

enum render_pipeline_parameter_type
{
    RENDER_PIPELINE_PARAMETER_TYPE_NODE,
    RENDER_PIPELINE_PARAMETER_TYPE_MATERIAL
};

class render_pass;
class render_pipeline : public render_node
{
public:
    using vertex_layout = std::vector<std::pair<std::string, rhi_resource_format>>;
    using parameter_layout =
        std::vector<std::pair<rhi_pipeline_parameter_layout*, render_pipeline_parameter_type>>;

public:
    render_pipeline(std::string_view name, rhi_context* rhi);
    virtual ~render_pipeline();

    void set_shader(std::string_view vertex, std::string_view pixel);

    void set_vertex_layout(const vertex_layout& vertex_layout);
    const vertex_layout& get_vertex_layout() const noexcept;

    void set_parameter_layout(const parameter_layout& parameter_layout);
    rhi_pipeline_parameter_layout* get_parameter_layout(
        render_pipeline_parameter_type type) const noexcept;

    void set_blend(const rhi_blend_desc& blend) noexcept;
    void set_depth_stencil(const rhi_depth_stencil_desc& depth_stencil) noexcept;
    void set_cull_mode(rhi_cull_mode cull_mode) noexcept;

    void set_samples(rhi_sample_count samples) noexcept;
    void set_primitive_topology(rhi_primitive_topology primitive_topology) noexcept;

    bool compile(rhi_render_pass* render_pass, std::size_t subpass_index);
    void execute(rhi_render_command* command);

    void add_mesh(const render_mesh& mesh);
    void set_camera_parameter(rhi_pipeline_parameter* parameter) noexcept;

    rhi_render_pipeline* get_interface() const noexcept { return m_interface; }

private:
    virtual void render(rhi_render_command* command, render_context& context);

    std::string m_vertex_shader;
    std::string m_pixel_shader;
    vertex_layout m_vertex_layout;
    parameter_layout m_parameter_layout;

    rhi_render_pipeline_desc m_desc;

    rhi_render_pipeline* m_interface;

    render_context m_context;
};
} // namespace violet