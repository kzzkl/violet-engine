#include "physics_plugin.hpp"
#include "common/log.hpp"

namespace violet
{
physics_plugin::physics_plugin() : m_pei(nullptr)
{
}

bool physics_plugin::on_load()
{
    create_pei m_create_func = static_cast<create_pei>(find_symbol("create_pei"));
    if (m_create_func == nullptr)
    {
        log::error("Symbol not found in plugin: create_pei.");
        return false;
    }

    destroy_pei m_destroy_func = static_cast<destroy_pei>(find_symbol("destroy_pei"));
    if (m_create_func == nullptr)
    {
        log::error("Symbol not found in plugin: destroy_pei.");
        return false;
    }

    m_pei = m_create_func();

    return true;
}

void physics_plugin::on_unload()
{
    m_destroy_func(m_pei);
}
} // namespace violet