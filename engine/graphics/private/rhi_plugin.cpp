#include "rhi_plugin.hpp"
#include "common/log.hpp"

namespace violet
{
rhi_plugin::rhi_plugin() : m_rhi(nullptr)
{
}

rhi_plugin::~rhi_plugin()
{
}

bool rhi_plugin::on_load()
{
    m_create_func = static_cast<create_rhi>(find_symbol("create_rhi"));
    if (m_create_func == nullptr)
    {
        log::error("Symbol not found in plugin: create_rhi.");
        return false;
    }

    m_destroy_func = static_cast<destroy_rhi>(find_symbol("destroy_rhi"));
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
    destroy_rhi(m_rhi);
}
} // namespace violet