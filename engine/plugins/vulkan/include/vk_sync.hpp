#pragma once

#include "vk_common.hpp"

namespace violet::vk
{
class vk_rhi;
class vk_fence : public rhi_fence
{
public:
    vk_fence(bool signaled, vk_rhi* rhi);
    vk_fence(const vk_fence&) = delete;
    virtual ~vk_fence();

    virtual void wait() override;

    VkFence get_fence() const noexcept { return m_fence; }

    vk_fence& operator=(const vk_fence&) = delete;

private:
    VkFence m_fence;
    vk_rhi* m_rhi;
};

class vk_semaphore : public rhi_semaphore
{
public:
    vk_semaphore(vk_rhi* rhi);
    vk_semaphore(const vk_semaphore&) = delete;
    virtual ~vk_semaphore();

    VkSemaphore get_semaphore() const noexcept { return m_semaphore; }

    vk_semaphore& operator=(const vk_semaphore&) = delete;

private:
    VkSemaphore m_semaphore;
    vk_rhi* m_rhi;
};
} // namespace violet::vk