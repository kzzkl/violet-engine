#pragma once

#include "graphics/render_interface.hpp"
#include <queue>

namespace violet::vk
{
class vk_context;
class vk_deletion_queue
{
public:
    vk_deletion_queue(vk_context* context) noexcept;

    void tick(std::size_t frame_index);

    void flush();

    void push(rhi_render_pass* render_pass);
    void push(rhi_render_pipeline* render_pipeline);
    void push(rhi_compute_pipeline* compute_pipeline);
    void push(rhi_parameter* parameter);
    void push(rhi_texture_srv* srv);
    void push(rhi_texture_uav* uav);
    void push(rhi_texture_rtv* rtv);
    void push(rhi_texture_dsv* dsv);
    void push(rhi_buffer_srv* srv);
    void push(rhi_buffer_uav* uav);
    void push(rhi_sampler* sampler);
    void push(rhi_buffer* buffer);
    void push(rhi_texture* texture);
    void push(rhi_swapchain* swapchain);
    void push(rhi_fence* fence);

private:
    template <typename T>
    class sub_queue
    {
    public:
        void tick(std::size_t frame_index)
        {
            while (!m_queue.empty())
            {
                if (m_queue.front().second <= frame_index)
                {
                    delete m_queue.front().first;
                    m_queue.pop();
                }
                else
                {
                    break;
                }
            }
        }

        void push(T* object, std::size_t frame_index)
        {
            m_queue.push({object, frame_index});
        }

        void flush()
        {
            while (!m_queue.empty())
            {
                delete m_queue.front().first;
                m_queue.pop();
            }
        }

    private:
        std::queue<std::pair<T*, std::size_t>> m_queue;
    };

    std::size_t get_delete_frame() const noexcept;

    sub_queue<rhi_render_pass> m_render_pass_queue;
    sub_queue<rhi_render_pipeline> m_render_pipeline_queue;
    sub_queue<rhi_compute_pipeline> m_compute_pipeline_queue;
    sub_queue<rhi_parameter> m_parameter_queue;
    sub_queue<rhi_descriptor> m_descriptor_queue;
    sub_queue<rhi_buffer> m_buffer_queue;
    sub_queue<rhi_texture> m_texture_queue;
    sub_queue<rhi_swapchain> m_swapchain_queue;
    sub_queue<rhi_fence> m_fence_queue;

    vk_context* m_context;
};
} // namespace violet::vk