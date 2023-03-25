#include "rhi/rhi_plugin.hpp"
#include "common/log.hpp"

namespace violet
{
bool rhi_plugin::on_load()
{
    make_rhi make = static_cast<make_rhi>(find_symbol("make_rhi"));
    if (make == nullptr)
    {
        log::error("Symbol not found in plugin: make_rhi.");
        return false;
    }

    m_rhi_impl.reset(make());

    return true;
}

void rhi_plugin::on_unload()
{
    m_rhi_impl = nullptr;
}
} // namespace violet