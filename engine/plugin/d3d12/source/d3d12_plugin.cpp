#include "d3d12_renderer.hpp"
#include <cstring>

namespace ash::graphics::d3d12
{
class d3d12_graphics_factory : public ash::graphics::external::graphics_factory
{
public:
    virtual ash::graphics::external::renderer* make_renderer() override { return nullptr; }
};
} // namespace ash::graphics::d3d12

extern "C"
{
    PLUGIN_API ash::core::external::plugin_info get_plugin_info()
    {
        using namespace ash::core::external;
        plugin_info info = {};

        char name[] = "graphics-d3d12";
        memcpy(info.name, name, sizeof(name));

        info.version.major = 1;
        info.version.minor = 0;

        return info;
    }

    PLUGIN_API ash::graphics::external::graphics_factory* make_factory()
    {
        return new ash::graphics::d3d12::d3d12_graphics_factory();
    }
}