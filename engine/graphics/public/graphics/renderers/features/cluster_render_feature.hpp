#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class cluster_render_feature : public render_feature<cluster_render_feature>
{
public:
    rhi_buffer* get_cluster_queue() const noexcept
    {
        return m_cluster_queue.get();
    }

    rhi_buffer* get_cluster_queue_state() const noexcept
    {
        return m_cluster_queue_state.get();
    }

    std::uint32_t get_max_clusters() const noexcept;
    std::uint32_t get_max_cluster_nodes() const noexcept;

    void set_persistent_thread(bool use_persistent_thread) noexcept
    {
        m_use_persistent_thread = use_persistent_thread;
    }

    bool use_persistent_thread() const noexcept
    {
        return m_use_persistent_thread;
    }

    bool need_initialize() const noexcept
    {
        return m_frame == 1;
    }

private:
    void on_update(std::uint32_t width, std::uint32_t height) override
    {
        ++m_frame;
    }

    void on_enable() override;

    void on_disable() override
    {
        m_cluster_queue = nullptr;
        m_cluster_queue_state = nullptr;
    }

    rhi_ptr<rhi_buffer> m_cluster_queue;
    rhi_ptr<rhi_buffer> m_cluster_queue_state;

    bool m_use_persistent_thread{false};

    std::size_t m_frame{0};
};
} // namespace violet