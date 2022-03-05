#include "d3d12_context.hpp"
#include "d3d12_diagnotor.hpp"
#include "d3d12_renderer.hpp"
#include <cstring>

using namespace ash::graphics::external;
using namespace ash::graphics::d3d12;

namespace ash::graphics::d3d12
{
class d3d12_graphics_factory : public graphics_factory
{
public:
};

class d3d12_context_wrapper : public graphics_context
{
public:
    d3d12_context_wrapper() : m_factory(std::make_unique<d3d12_graphics_factory>()) {}

    virtual bool initialize(const graphics_context_config& config) override
    {
        return d3d12_context::instance().initialize(config);
    }

    virtual graphics_factory* get_factory() override { return m_factory.get(); }
    virtual diagnotor* get_diagnotor() override
    {
        return d3d12_context::instance().get_diagnotor();
    }
    virtual renderer* get_renderer() override { return d3d12_context::instance().get_renderer(); }

private:
    std::unique_ptr<d3d12_graphics_factory> m_factory;
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

    PLUGIN_API d3d12_context_wrapper* make_context() { return new d3d12_context_wrapper(); }
}