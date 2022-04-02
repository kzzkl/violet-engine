#include "physics_plugin.hpp"
#include "log.hpp"

namespace ash::physics
{
bool physics_plugin::do_load()
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

void physics_plugin::do_unload()
{
}
} // namespace ash::physics