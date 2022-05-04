#include "vk_common.hpp"
#include "vk_context.hpp"
#include "vk_frame_buffer.hpp"
#include "vk_renderer.hpp"
#include "vk_resource.hpp"

namespace ash::graphics::vk
{
class vk_factory : public factory_interface
{
public:
    virtual renderer_interface* make_renderer(const renderer_desc& desc) override
    {
        return new vk_renderer(desc);
    }

    virtual render_target_set_interface* make_render_target_set(
        const render_target_set_desc& desc) override
    {
        return new vk_frame_buffer(desc);
    }

    virtual technique_interface* make_technique(const technique_desc& desc) override
    {
        return new vk_render_pass(desc);
    }

    virtual pass_parameter_layout_interface* make_pass_parameter_layout(
        const pass_parameter_layout_desc& desc) override
    {
        return new vk_pass_parameter_layout(desc);
    }

    virtual pass_layout_interface* make_pass_layout(
        const pass_layout_desc& desc) override
    {
        return new vk_pass_layout(desc);
    }

    virtual pass_parameter_interface* make_pass_parameter(
        pass_parameter_layout_interface* layout) override
    {
        return new vk_pass_parameter(layout);
    }

    virtual resource_interface* make_vertex_buffer(const vertex_buffer_desc& desc) override
    {
        return new vk_vertex_buffer(desc);
    }

    virtual resource_interface* make_index_buffer(const index_buffer_desc& desc) override
    {
        return new vk_index_buffer(desc);
    }

    resource_interface* make_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height) override
    {
        return nullptr;
    }

    virtual resource_interface* make_texture(const char* file) override
    {
        return new vk_texture(file);
    }

    virtual resource_interface* make_render_target(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling) override
    {
        return nullptr;
    }

    virtual resource_interface* make_depth_stencil(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling) override
    {
        return new vk_depth_stencil_buffer(width, height, multiple_sampling);
    }
};
} // namespace ash::graphics::vk

extern "C"
{
    PLUGIN_API ash::core::plugin_info get_plugin_info()
    {
        ash::core::plugin_info info = {};

        char name[] = "graphics-vulkan";
        memcpy(info.name, name, sizeof(name));

        info.version.major = 1;
        info.version.minor = 0;

        return info;
    }

    PLUGIN_API ash::graphics::factory_interface* make_factory()
    {
        return new ash::graphics::vk::vk_factory();
    }
}