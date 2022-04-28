#include "graphics_interface.hpp"
#include "vk_context.hpp"
#include "vk_renderer.hpp"

namespace ash::graphics::vk
{
class vk_context_wrapper : public context
{
public:
    virtual bool initialize(const context_config& config) override
    {
        return vk_context::initialize(config);
    }

    virtual void deinitialize() override { return vk_context::deinitialize(); }

    virtual renderer_type* renderer() override { return &vk_context::renderer(); }
    virtual factory_type* factory() { return nullptr; }

private:
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

    PLUGIN_API ash::graphics::context* make_context()
    {
        return new ash::graphics::vk::vk_context_wrapper();
    }
}