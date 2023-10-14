#pragma once

#include "vk_context.hpp"

namespace violet::vk
{
class vk_fence : public rhi_fence
{
public:
    vk_fence(bool signaled, vk_context* context);
    vk_fence(const vk_fence&) = delete;
    virtual ~vk_fence();

    virtual void wait() override;

    VkFence get_fence() const noexcept { return m_fence; }

    vk_fence& operator=(const vk_fence&) = delete;

private:
    VkFence m_fence;
    vk_context* m_context;
};

class vk_semaphore : public rhi_semaphore
{
public:
    vk_semaphore(vk_context* context);
    vk_semaphore(const vk_semaphore&) = delete;
    virtual ~vk_semaphore();

    VkSemaphore get_semaphore() const noexcept { return m_semaphore; }

    vk_semaphore& operator=(const vk_semaphore&) = delete;

private:
    VkSemaphore m_semaphore;
    vk_context* m_context;
};
} // namespace violet::vk