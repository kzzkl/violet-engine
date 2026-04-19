#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class ssgi_feature : public render_feature<ssgi_feature>
{
public:
    float thickness{0.1f};
    std::uint32_t iteration_count{128};

    bool bilateral_denoise{true};
    float bilateral_blur_factor{0.25f};

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
        rhi_extent extent = {
            .width = width / 2,
            .height = height / 2,
        };

        if (m_history == nullptr || m_history->get_extent() != extent)
        {
            m_history = render_device::instance().create_texture({
                .extent = extent,
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

    void on_enable() override
    {
        m_history_valid = false;
    }

    void on_disable() override
    {
        m_history = nullptr;
        m_history_valid = false;
    }

    rhi_ptr<rhi_texture> m_history;
    bool m_history_valid{false};
};
} // namespace violet