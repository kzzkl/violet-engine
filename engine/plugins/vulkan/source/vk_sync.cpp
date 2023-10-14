#include "vk_sync.hpp"

namespace violet::vk
{
vk_fence::vk_fence(bool signaled, vk_context* context) : m_context(context)
{
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    throw_if_failed(vkCreateFence(m_context->get_device(), &fence_info, nullptr, &m_fence));
}

vk_fence::~vk_fence()
{
    vkDestroyFence(m_context->get_device(), m_fence, nullptr);
}

void vk_fence::wait()
{
    vkWaitForFences(m_context->get_device(), 1, &m_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_context->get_device(), 1, &m_fence);
}

vk_semaphore::vk_semaphore(vk_context* context) : m_context(context)
{
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    throw_if_failed(
        vkCreateSemaphore(m_context->get_device(), &semaphore_info, nullptr, &m_semaphore));
}

vk_semaphore::~vk_semaphore()
{
    vkDestroySemaphore(m_context->get_device(), m_semaphore, nullptr);
}
} // namespace violet::vk