#include "rhi_plugin.hpp"
#include "common/log.hpp"

namespace violet
{
bool rhi_plugin::on_load()
{
    create_rhi create = static_cast<create_rhi>(find_symbol("create_rhi"));
    if (create == nullptr)
    {
        log::error("Symbol not found in plugin: create_rhi.");
        return false;
    }

    m_rhi.reset(create());

    return true;
}

void rhi_plugin::on_unload()
{
    m_rhi = nullptr;
}
} // namespace violet