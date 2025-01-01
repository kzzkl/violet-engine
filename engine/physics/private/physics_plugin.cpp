#include "physics_plugin.hpp"
#include "common/log.hpp"

namespace violet
{
physics_plugin::physics_plugin()
    : m_plugin(nullptr)
{
}

bool physics_plugin::on_load()
{
    auto m_create_func = reinterpret_cast<phy_create_plugin>(find_symbol("phy_create_plugin"));
    if (m_create_func == nullptr)
    {
        log::error("Symbol not found in plugin: create_plugin.");
        return false;
    }

    auto m_destroy_func = reinterpret_cast<phy_destroy_plugin>(find_symbol("phy_destroy_plugin"));
    if (m_create_func == nullptr)
    {
        log::error("Symbol not found in plugin: destroy_plugin.");
        return false;
    }

    m_plugin = m_create_func();

    return true;
}

void physics_plugin::on_unload()
{
    m_destroy_func(m_plugin);
}
} // namespace violet