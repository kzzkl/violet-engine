#include "vk_sync.hpp"
#include "vk_rhi.hpp"

namespace violet::vk
{
vk_fence::vk_fence(bool signaled, vk_rhi* rhi) : m_rhi(rhi)
{
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    throw_if_failed(vkCreateFence(rhi->get_device(), &fence_info, nullptr, &m_fence));
}

vk_fence::~vk_fence()
{
    vkDestroyFence(m_rhi->get_device(), m_fence, nullptr);
}

void vk_fence::wait()
{
    vkWaitForFences(m_rhi->get_device(), 1, &m_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_rhi->get_device(), 1, &m_fence);
}

vk_semaphore::vk_semaphore(vk_rhi* rhi) : m_rhi(rhi)
{
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    throw_if_failed(vkCreateSemaphore(rhi->get_device(), &semaphore_info, nullptr, &m_semaphore));
}

vk_semaphore::~vk_semaphore()
{
    vkDestroySemaphore(m_rhi->get_device(), m_semaphore, nullptr);
}
} // namespace violet::vk