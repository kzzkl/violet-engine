#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class eye_adaptation_feature : public render_feature<eye_adaptation_feature>
{
public:
    float min_ev{-8.0f};
    float max_ev{4.0f};

    float low_percent{0.1f};
    float high_percent{0.9f};
    float min_brightness{0.001f};
    float max_brightness{10.0f};
    float speed_down{1.0f};
    float speed_up{3.0f};

    rhi_texture* get_exposure() const
    {
        return m_exposure.get();
    }

private:
    void on_enable() override
    {
        m_exposure = render_device::instance().create_texture({
            .extent = {.width = 1, .height = 1},
            .format = RHI_FORMAT_R32_FLOAT,
            .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE,
            .level_count = 1,
            .layer_count = 1,
            .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        });
    }

    void on_disable() override
    {
        m_exposure = nullptr;
    }

    rhi_ptr<rhi_texture> m_exposure;
};
} // namespace violet