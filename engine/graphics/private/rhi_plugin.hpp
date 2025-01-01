#pragma once

#include "core/plugin.hpp"
#include "graphics/render_interface.hpp"

namespace violet
{
class rhi_plugin : public plugin
{
public:
    rhi_plugin() = default;
    virtual ~rhi_plugin() = default;

    rhi* get_rhi() const noexcept
    {
        return m_rhi;
    }

protected:
    bool on_load() override;
    void on_unload() override;

private:
    create_rhi m_create_func;
    destroy_rhi m_destroy_func;

    rhi* m_rhi{nullptr};
};
} // namespace violet