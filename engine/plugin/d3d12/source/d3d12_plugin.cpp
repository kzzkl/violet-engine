#include "d3d12_command.hpp"
#include "d3d12_context.hpp"
#include "d3d12_pipeline.hpp"
#include "d3d12_renderer.hpp"
#include "d3d12_resource.hpp"
#include "graphics_interface.hpp"
#include <cstring>

namespace ash::graphics::d3d12
{
class d3d12_factory : public factory_interface
{
public:
    virtual renderer_interface* make_renderer(const renderer_desc& desc) override
    {
        return new d3d12_renderer(desc);
    }

    virtual render_pass_interface* make_render_pass(const render_pass_desc& desc) override
    {
        return new d3d12_render_pass(desc);
    }

    virtual compute_pipeline_interface* make_compute_pipeline(
        const compute_pipeline_desc& desc) override
    {
        return new d3d12_compute_pipeline(desc);
    }

    virtual pipeline_parameter_layout_interface* make_pipeline_parameter_layout(
        const pipeline_parameter_layout_desc& desc) override
    {
        return new d3d12_pipeline_parameter_layout(desc);
    }

    virtual pipeline_parameter_interface* make_pipeline_parameter(
        pipeline_parameter_layout_interface* layout) override
    {
        return new d3d12_pipeline_parameter(layout);
    }

    virtual resource_interface* make_vertex_buffer(const vertex_buffer_desc& desc) override
    {
        if (desc.dynamic)
        {
            return new d3d12_vertex_buffer_dynamic(desc);
        }
        else
        {
            auto command_list = d3d12_context::command()->allocate_dynamic_command();
            auto result = new d3d12_vertex_buffer_default(desc, command_list.get());
            d3d12_context::command()->execute_command(command_list);
            return result;
        }
    }

    virtual resource_interface* make_index_buffer(const index_buffer_desc& desc) override
    {
        if (desc.dynamic)
        {
            return new d3d12_index_buffer_dynamic(desc);
        }
        else
        {
            auto command_list = d3d12_context::command()->allocate_dynamic_command();
            auto result = new d3d12_index_buffer_default(desc, command_list.get());
            d3d12_context::command()->execute_command(command_list);
            return result;
        }
    }

    virtual resource_interface* make_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height,
        resource_format format) override
    {
        auto command_list = d3d12_context::command()->allocate_dynamic_command();
        d3d12_texture* result = new d3d12_texture(data, width, height, format, command_list.get());
        d3d12_context::command()->execute_command(command_list);
        return result;
    }

    virtual resource_interface* make_texture(const char* file) override
    {
        auto command_list = d3d12_context::command()->allocate_dynamic_command();
        d3d12_texture* result = new d3d12_texture(file, command_list.get());
        d3d12_context::command()->execute_command(command_list);
        return result;
    }

    virtual resource_interface* make_render_target(const render_target_desc& desc) override
    {
        return new d3d12_render_target(desc);
    }

    virtual resource_interface* make_depth_stencil_buffer(
        const depth_stencil_buffer_desc& desc) override
    {
        return new d3d12_depth_stencil_buffer(desc);
    }
};
} // namespace ash::graphics::d3d12

extern "C"
{
    PLUGIN_API ash::core::plugin_info get_plugin_info()
    {
        ash::core::plugin_info info = {};

        char name[] = "graphics-d3d12";
        memcpy(info.name, name, sizeof(name));

        info.version.major = 1;
        info.version.minor = 0;

        return info;
    }

    PLUGIN_API ash::graphics::factory_interface* make_factory()
    {
        return new ash::graphics::d3d12::d3d12_factory();
    }
}