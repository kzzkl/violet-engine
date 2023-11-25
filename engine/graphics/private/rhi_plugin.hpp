#pragma once

#include "core/plugin.hpp"
#include "graphics/render_interface.hpp"

namespace violet
{
class rhi_plugin : public plugin
{
public:
    rhi_plugin();
    virtual ~rhi_plugin();

    rhi_renderer* get_rhi() const noexcept { return m_rhi; }

protected:
    virtual bool on_load() override;
    virtual void on_unload() override;

private:
    create_rhi m_create_func;
    destroy_rhi m_destroy_func;

    rhi_renderer* m_rhi;
};
} // namespace violet