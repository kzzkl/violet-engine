#pragma once

#include "vk_context.hpp"

namespace violet::vk
{
class vk_fence : public rhi_fence
{
public:
    vk_fence(bool timeline, vk_context* context);
    vk_fence(const vk_fence&) = delete;
    virtual ~vk_fence();

    void wait(std::uint64_t value) override;

    VkSemaphore get_semaphore() const noexcept
    {
        return m_semaphore;
    }

    vk_fence& operator=(const vk_fence&) = delete;

private:
    VkSemaphore m_semaphore;
    vk_context* m_context;
};
} // namespace violet::vk