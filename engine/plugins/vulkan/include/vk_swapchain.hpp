#pragma once

#include "vk_common.hpp"
#include "vk_context.hpp"
#include "vk_resource.hpp"
#include "vk_sync.hpp"
#include <memory>
#include <vector>

namespace violet::vk
{
class vk_swapchain_image : public vk_texture
{
public:
    vk_swapchain_image(
        VkImage image,
        VkFormat format,
        const VkExtent2D& extent,
        vk_context* context);
    virtual ~vk_swapchain_image();
};

class vk_swapchain : public rhi_swapchain
{
public:
    vk_swapchain(const rhi_swapchain_desc& desc, vk_context* context);
    vk_swapchain(const vk_swapchain&) = delete;
    ~vk_swapchain();

    rhi_semaphore* acquire_texture() override;
    rhi_semaphore* get_present_semaphore() const override;

    void present() override;

    void resize(std::uint32_t width, std::uint32_t height) override;

    rhi_texture* get_texture() override;

    vk_swapchain_image* get_current_image() const
    {
        return m_swapchain_images[m_swapchain_image_index].get();
    }
    std::uint32_t get_image_index() const noexcept
    {
        return m_swapchain_image_index;
    }
    VkSwapchainKHR get_swapchain() const noexcept
    {
        return m_swapchain;
    }

    vk_swapchain& operator=(const vk_swapchain&) = delete;

private:
    VkSwapchainKHR m_swapchain;
    VkSurfaceKHR m_surface;

    std::vector<std::unique_ptr<vk_swapchain_image>> m_swapchain_images;
    std::uint32_t m_swapchain_image_index;

    std::vector<std::unique_ptr<vk_semaphore>> m_available_semaphores;
    std::vector<std::unique_ptr<vk_semaphore>> m_present_semaphores;

    vk_context* m_context;
};
} // namespace violet::vk