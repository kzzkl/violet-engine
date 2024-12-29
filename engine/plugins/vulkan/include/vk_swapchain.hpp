#pragma once

#include "vk_context.hpp"
#include "vk_resource.hpp"
#include "vk_sync.hpp"
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

    rhi_format get_format() const noexcept override
    {
        return m_format;
    }

    rhi_sample_count get_samples() const noexcept override
    {
        return RHI_SAMPLE_COUNT_1;
    }

    rhi_texture_extent get_extent() const noexcept override
    {
        return m_extent;
    }

    std::uint32_t get_level() const noexcept override
    {
        return 0;
    }

    std::uint32_t get_level_count() const noexcept override
    {
        return 1;
    }

    std::uint32_t get_layer() const noexcept override
    {
        return 0;
    }

    std::uint32_t get_layer_count() const noexcept override
    {
        return 1;
    }

    std::uint64_t get_hash() const noexcept override
    {
        return m_hash;
    }

    VkImage get_image() const noexcept override
    {
        return m_image;
    }

    VkImageView get_image_view() const noexcept override
    {
        return m_image_view;
    }

    VkClearValue get_clear_value() const noexcept override
    {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }

    VkImageAspectFlags get_aspect_mask() const noexcept override
    {
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

private:
    VkImage m_image{VK_NULL_HANDLE};
    VkImageView m_image_view{VK_NULL_HANDLE};

    rhi_format m_format{RHI_FORMAT_UNDEFINED};
    rhi_texture_extent m_extent;
    std::uint64_t m_hash;

    vk_context* m_context{nullptr};
};

class vk_swapchain : public rhi_swapchain
{
public:
    vk_swapchain(const rhi_swapchain_desc& desc, vk_context* context);
    vk_swapchain(const vk_swapchain&) = delete;
    ~vk_swapchain();

    rhi_fence* acquire_texture() override;
    rhi_fence* get_present_fence() const override;

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

    std::vector<std::unique_ptr<vk_fence>> m_available_semaphores;
    std::vector<std::unique_ptr<vk_fence>> m_present_semaphores;

    rhi_texture_flags m_flags{0};

    vk_context* m_context{nullptr};
};
} // namespace violet::vk