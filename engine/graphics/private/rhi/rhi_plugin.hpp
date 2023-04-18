#pragma once

#include "core/context/plugin.hpp"
#include "interface/graphics_interface.hpp"

namespace violet
{
class rhi_plugin : public plugin
{
public:
    rhi_plugin() {}

    rhi_interface* get_rhi() const noexcept { return m_rhi.get(); }

protected:
    virtual bool on_load() override;
    virtual void on_unload() override;

private:
    std::unique_ptr<rhi_interface> m_rhi;
};
} // namespace violet