#include "vk_common.hpp"
#include "vk_context.hpp"
#include "vk_frame_buffer.hpp"
#include "vk_renderer.hpp"
#include "vk_resource.hpp"

namespace ash::graphics::vk
{
class vk_factory : public factory
{
public:
    virtual frame_buffer* make_frame_buffer(const frame_buffer_desc& desc) override
    {
        return new vk_frame_buffer(desc);
    }

    virtual render_pass* make_render_pass(const render_pass_desc& desc) override
    {
        return new vk_render_pass(desc);
    }

    virtual renderer* make_renderer(const renderer_desc& desc) override
    {
        return new vk_renderer(desc);
    }

    virtual resource* make_vertex_buffer(const vertex_buffer_desc& desc) override
    {
        return new vk_vertex_buffer(desc);
    }

    virtual resource* make_index_buffer(const index_buffer_desc& desc) override
    {
        return new vk_index_buffer(desc);
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

    PLUGIN_API ash::graphics::vk::factory* make_factory()
    {
        return new ash::graphics::vk::vk_factory();
    }
}