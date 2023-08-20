#pragma once

#include "core/plugin.hpp"
#include "graphics/rhi.hpp"

namespace violet
{
class rhi_plugin : public plugin
{
public:
    rhi_plugin() {}

    rhi_context* get_rhi() const noexcept { return m_rhi.get(); }

protected:
    virtual bool on_load() override;
    virtual void on_unload() override;

private:
    std::unique_ptr<rhi_context> m_rhi;
};
} // namespace violet