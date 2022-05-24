#include "graphics/graphics_plugin.hpp"
#include "log.hpp"

using namespace ash::core;

namespace ash::graphics
{
graphics_plugin::graphics_plugin()
{
}

bool graphics_plugin::on_load()
{
    make_factory make = static_cast<make_factory>(find_symbol("make_factory"));
    if (make == nullptr)
    {
        log::error("Symbol not found in plugin: make_factory.");
        return false;
    }

    m_factory.reset(make());

    return true;
}

void graphics_plugin::on_unload()
{
    m_factory = nullptr;
}
} // namespace ash::graphics