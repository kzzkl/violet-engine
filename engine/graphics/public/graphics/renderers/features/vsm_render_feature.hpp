#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class vsm_render_feature : public render_feature<vsm_render_feature>
{
public:
    struct debug_info
    {
        std::uint32_t cache_hit;
        std::uint32_t rendered;
        std::uint32_t unmapped;
        std::uint32_t static_drawcall;
        std::uint32_t dynamic_drawcall;
    };

    vsm_render_feature();

    rhi_buffer* get_lru_state() const noexcept
    {
        return m_lru_state.get();
    }

    rhi_buffer* get_lru_buffer() const noexcept
    {
        return m_lru_buffer.get();
    }

    std::uint32_t get_curr_lru_index() const noexcept
    {
        return (m_frame + 1) % 2;
    }

    std::uint32_t get_prev_lru_index() const noexcept
    {
        return m_frame % 2;
    }

    void set_debug_info(bool enable);

    debug_info get_debug_info() const;

    rhi_buffer* get_debug_info_buffer() const noexcept
    {
        return m_debug_info.get();
    }

private:
    void on_update(std::uint32_t width, std::uint32_t height) override
    {
        ++m_frame;
    }

    rhi_ptr<rhi_buffer> m_lru_state{nullptr};
    rhi_ptr<rhi_buffer> m_lru_buffer{nullptr};

    rhi_ptr<rhi_buffer> m_debug_info{nullptr};

    std::uint32_t m_frame{0};
};
} // namespace violet