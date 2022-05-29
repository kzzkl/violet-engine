#pragma once

#include "graphics_interface.hpp"
#include <string>
#include <vector>

namespace ash::graphics
{
class pipeline_parameter_layout_info
{
public:
    pipeline_parameter_layout_desc convert() noexcept;
    std::vector<pipeline_parameter_pair> parameters;
};

class pipeline_info
{
public:
    pipeline_info();

    pipeline_desc convert() noexcept;

    std::string vertex_shader;
    std::string pixel_shader;

    std::vector<vertex_attribute> vertex_attributes;
    std::vector<std::string> parameters;
    std::vector<pipeline_parameter_layout_interface*> parameter_interfaces;

    blend_desc blend;
    depth_stencil_desc depth_stencil;
    rasterizer_desc rasterizer;

    std::vector<attachment_reference> references;

    primitive_topology_type primitive_topology;
    std::size_t samples;
};

class attachment_info : public attachment_desc
{
public:
    attachment_desc convert() noexcept { return *this; }
};

class render_pass_info
{
public:
    render_pass_desc convert() noexcept;

    std::vector<attachment_info> attachments;
    std::vector<pipeline_info> subpasses;

private:
    std::vector<attachment_desc> m_attachment_desc;
    std::vector<pipeline_desc> m_pass_desc;
};

class compute_pipeline_info
{
public:
    compute_pipeline_desc convert() noexcept;

    std::string compute_shader;
    std::vector<std::string> parameters;
    std::vector<pipeline_parameter_layout_interface*> parameter_interfaces;
};

class renderer_info : public renderer_desc
{
public:
    renderer_desc convert() noexcept { return *this; }
};

class vertex_buffer_info : public vertex_buffer_desc
{
public:
    vertex_buffer_desc convert() noexcept { return *this; }
};

class index_buffer_info : public index_buffer_desc
{
public:
    index_buffer_desc convert() noexcept { return *this; }
};

using render_target_info = render_target_desc;
using depth_stencil_buffer_info = depth_stencil_buffer_desc;
} // namespace ash::graphics