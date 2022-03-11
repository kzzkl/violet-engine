#include "graphics_plugin.hpp"
#include "log.hpp"

using namespace ash::core;
using namespace ash::common;

namespace ash::graphics
{
graphics_plugin::graphics_plugin()
{
}

bool graphics_plugin::initialize(const context_config& config)
{
    return m_context->initialize(config);
}

bool graphics_plugin::do_load()
{
    make_context make = static_cast<make_context>(find_symbol("make_context"));
    if (make == nullptr)
    {
        log::error("Symbol not found in plugin: make_context.");
        return false;
    }

    m_context.reset(make());

    return true;
}

void graphics_plugin::do_unload()
{
}
} // namespace ash::graphics