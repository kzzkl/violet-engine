#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class taa_feature : public render_feature<taa_feature>
{
public:
    rhi_texture* get_history()
    {
        return m_history.get();
    }

    bool is_history_valid() const noexcept
    {
        return m_history_valid;
    }

private:
    void on_update(std::uint32_t width, std::uint32_t height) override
    {
        if (m_history == nullptr || m_history->get_extent().width != width ||
            m_history->get_extent().height != height)
        {
            m_history = render_device::instance().create_texture({
                .extent = {.width = width, .height = height},
                .format = RHI_FORMAT_R16G16B16A16_FLOAT,
                .flags =
                    RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE | RHI_TEXTURE_TRANSFER_DST,
                .level_count = 1,
                .layer_count = 1,
                .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            });

            m_history_valid = false;
        }
        else
        {
            m_history_valid = true;
        }
    }

    rhi_ptr<rhi_texture> m_history;
    bool m_history_valid{false};
};
} // namespace violet