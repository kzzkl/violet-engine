#pragma once

#include "vk_common.hpp"
#include "vk_context.hpp"
#include "vk_resource.hpp"
#include <memory>
#include <vector>

namespace violet::vk
{
class vk_swapchain_image : public vk_image
{
public:
    vk_swapchain_image(
        VkImage image,
        VkFormat format,
        const VkExtent2D& extent,
        vk_context* context);
    virtual ~vk_swapchain_image();
};

class vk_swapchain
{
public:
    vk_swapchain(
        std::uint32_t width,
        std::uint32_t height,
        const std::vector<std::uint32_t>& queue_family_indices,
        vk_context* context);
    vk_swapchain(const vk_swapchain&) = delete;
    ~vk_swapchain();

    std::uint32_t acquire_next_image(VkSemaphore signal_semaphore);

    void resize(std::uint32_t width, std::uint32_t height);

    vk_swapchain_image* get_current_image() const
    {
        return m_swapchain_images[m_swapchain_image_index].get();
    }
    std::uint32_t get_image_index() const noexcept { return m_swapchain_image_index; }
    VkSwapchainKHR get_swapchain() const noexcept { return m_swapchain; }

    vk_swapchain& operator=(const vk_swapchain&) = delete;

private:
    VkSwapchainKHR m_swapchain;

    std::vector<std::unique_ptr<vk_swapchain_image>> m_swapchain_images;
    std::uint32_t m_swapchain_image_index;

    vk_context* m_context;
    std::vector<std::uint32_t> m_queue_family_indices;
};
} // namespace violet::vk