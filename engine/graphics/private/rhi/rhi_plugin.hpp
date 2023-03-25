#pragma once

#include "core/context/plugin.hpp"
#include "graphics_interface.hpp"

namespace violet
{
class rhi_plugin : public plugin
{
public:
    rhi_plugin() {}

    rhi_interface* get_rhi_impl() const noexcept { return m_rhi_impl.get(); }

protected:
    virtual bool on_load() override;
    virtual void on_unload() override;

private:
    std::unique_ptr<rhi_interface> m_rhi_impl;
};
} // namespace violet