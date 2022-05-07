#pragma once

#include "graphics_interface.hpp"
#include <string>
#include <vector>

namespace ash::graphics
{
class pipeline_layout_info
{
public:
    pipeline_layout_desc convert() noexcept;
    std::vector<pipeline_parameter_pair> parameters;
};

class blend_info : public blend_desc
{
public:
    blend_info();

    blend_desc convert() noexcept { return *this; }
};

class depth_stencil_info : public depth_stencil_desc
{
public:
    depth_stencil_desc convert() noexcept { return *this; }
};

class pipeline_info
{
public:
    pipeline_info();

    pipeline_desc convert() noexcept;

    std::string vertex_shader;
    std::string pixel_shader;

    std::vector<vertex_attribute_type> vertex_attributes;
    std::vector<std::string> parameters;
    std::vector<pipeline_layout_interface*> parameter_interfaces;

    blend_info blend;
    depth_stencil_info depth_stencil;

    std::vector<attachment_reference> references;

    primitive_topology primitive_topology;
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

class attachment_set_info
{
public:
    attachment_set_desc convert() noexcept;

    std::vector<resource_interface*> attachments;
    std::uint32_t width;
    std::uint32_t height;
    render_pass_interface* render_pass;
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
} // namespace ash::graphics