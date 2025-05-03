#include "rhi_plugin.hpp"
#include "common/log.hpp"

namespace violet
{
bool rhi_plugin::on_load()
{
    m_create_func = reinterpret_cast<create_rhi>(find_symbol("create_rhi"));
    if (m_create_func == nullptr)
    {
        log::error("Symbol not found in plugin: create_rhi.");
        return false;
    }

    m_destroy_func = reinterpret_cast<destroy_rhi>(find_symbol("destroy_rhi"));
    if (m_destroy_func == nullptr)
    {
        log::error("Symbol not found in plugin: destroy_rhi.");
        return false;
    }

    m_rhi = m_create_func();

    return true;
}

void rhi_plugin::on_unload()
{
    m_destroy_func(m_rhi);
}
} // namespace violet