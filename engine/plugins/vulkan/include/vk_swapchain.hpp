#pragma once

#include "vk_context.hpp"
#include "vk_resource.hpp"
#include "vk_sync.hpp"
#include <memory>
#include <vector>

namespace violet::vk
{
class vk_swapchain : public rhi_swapchain
{
public:
    vk_swapchain(const rhi_swapchain_desc& desc, vk_context* context);
    vk_swapchain(const vk_swapchain&) = delete;
    ~vk_swapchain();

    rhi_fence* acquire_texture() override;
    rhi_fence* get_present_fence() const override;

    void present() override;

    void resize() override
    {
        m_resized = true;
    }

    rhi_texture* get_texture() override
    {
        return m_swapchain_images[m_swapchain_image_index].get();
    }

    rhi_texture* get_texture(std::size_t index) override
    {
        return m_swapchain_images[index].get();
    }

    std::size_t get_texture_count() const noexcept override
    {
        return m_swapchain_images.size();
    }

    VkSwapchainKHR get_swapchain() const noexcept
    {
        return m_swapchain;
    }

    vk_swapchain& operator=(const vk_swapchain&) = delete;

private:
    void update();

    VkSwapchainKHR m_swapchain;
    VkSurfaceKHR m_surface;

    std::vector<std::unique_ptr<vk_texture>> m_swapchain_images;
    std::uint32_t m_swapchain_image_index;

    std::vector<std::unique_ptr<vk_fence>> m_available_semaphores;
    std::vector<std::unique_ptr<vk_fence>> m_present_semaphores;

    rhi_texture_flags m_flags{0};

    bool m_resized{false};

    vk_context* m_context{nullptr};
};
} // namespace violet::vk