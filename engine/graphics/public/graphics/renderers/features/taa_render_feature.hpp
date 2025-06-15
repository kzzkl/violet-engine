#pragma once

#include "graphics/renderer.hpp"
#include <array>

namespace violet
{
class taa_render_feature : public render_feature<taa_render_feature>
{
public:
    rhi_texture* get_current()
    {
        return m_history[m_frame % 2].get();
    }

    rhi_texture* get_history()
    {
        return m_history[(m_frame + 1) % 2].get();
    }

    bool is_history_valid() const noexcept
    {
        // TODO: Confirm the issue of screen artifacts when resizing the window when using the
        // condition m_frame > 0.
        return m_frame > 1; // m_frame > 0;
    }

private:
    void on_update(std::uint32_t width, std::uint32_t height) override
    {
        bool recreate = false;

        if (m_history[0] == nullptr)
        {
            recreate = true;
        }
        else
        {
            auto extent = m_history[0]->get_extent();
            recreate = extent.width != width || extent.height != height;
        }

        if (recreate)
        {
            for (auto& target : m_history)
            {
                target = render_device::instance().create_texture({
                    .extent = {width, height},
                    .format = RHI_FORMAT_R16G16B16A16_FLOAT,
                    .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE,
                    .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
                });
            }

            m_frame = 0;
        }
        else
        {
            ++m_frame;
        }
    }

    void on_disable() override
    {
        m_frame = 0;
    }

    std::array<rhi_ptr<rhi_texture>, 2> m_history;

    std::size_t m_frame{0};
};
} // namespace violet