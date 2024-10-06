#include "vk_sync.hpp"

namespace violet::vk
{
vk_fence::vk_fence(bool timeline, vk_context* context)
    : m_context(context)
{
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (timeline)
    {
        VkSemaphoreTypeCreateInfo timeline_info = {};
        timeline_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timeline_info.pNext = NULL;
        timeline_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timeline_info.initialValue = 0;

        semaphore_info.pNext = &timeline_info;
    }

    vk_check(vkCreateSemaphore(m_context->get_device(), &semaphore_info, nullptr, &m_semaphore));
}

vk_fence::~vk_fence()
{
    vkDestroySemaphore(m_context->get_device(), m_semaphore, nullptr);
}

void vk_fence::wait(std::uint64_t value)
{
    VkSemaphoreWaitInfo wait_info = {};
    wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    wait_info.pNext = NULL;
    wait_info.flags = 0;
    wait_info.semaphoreCount = 1;
    wait_info.pSemaphores = &m_semaphore;
    wait_info.pValues = &value;

    vkWaitSemaphores(m_context->get_device(), &wait_info, UINT64_MAX);
}
} // namespace violet::vk