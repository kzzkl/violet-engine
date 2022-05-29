#include "physics/physics_plugin.hpp"
#include "log.hpp"

namespace ash::physics
{
physics_plugin::physics_plugin()
{
}

bool physics_plugin::on_load()
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

void physics_plugin::on_unload()
{
    m_factory = nullptr;
}
} // namespace ash::physics