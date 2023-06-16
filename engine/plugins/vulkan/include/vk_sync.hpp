#pragma once

#include "vk_common.hpp"

namespace violet::vk
{
class vk_rhi;
class vk_fence : public rhi_fence
{
public:
    vk_fence(vk_rhi* rhi);
    virtual ~vk_fence();

    virtual void wait() override;

    VkFence get_fence() const noexcept { return m_fence; }

private:
    VkFence m_fence;
    vk_rhi* m_rhi;
};

class vk_semaphore : public rhi_semaphore
{
public:
    vk_semaphore(vk_rhi* rhi);
    virtual ~vk_semaphore();

    VkSemaphore get_semaphore() const noexcept { return m_semaphore; }

private:
    VkSemaphore m_semaphore;
    vk_rhi* m_rhi;
};
} // namespace violet::vk