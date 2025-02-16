#pragma once

#include "graphics/render_device.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include <array>

namespace violet
{
class taa_pass
{
public:
    struct parameter
    {
        rdg_texture* current_render_target;
        rdg_texture* history_render_target;
        rdg_texture* depth_buffer;
        rdg_texture* motion_vector;
        rdg_texture* resolved_render_target;
    };

    static void add(render_graph& graph, const parameter& parameter);
};

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
        // TODO: Confirm the issue of screen artifacts when resizing the window when using the condition m_frame > 0.
        return m_frame > 1; // m_frame > 0;
    }

private:
    void on_update(std::uint32_t width, std::uint32_t height) override;
    void on_disable() override
    {
        m_frame = 0;
    }

    std::array<rhi_ptr<rhi_texture>, 2> m_history;

    std::size_t m_frame;
};
} // namespace violet