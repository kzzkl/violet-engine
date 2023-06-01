#pragma once

#include "vk_common.hpp"

namespace violet::vk
{
class vk_rhi;
class vk_resource : public rhi_resource
{
public:
    vk_resource() = default;
    virtual ~vk_resource() = default;

    virtual resource_format get_format() const noexcept override
    {
        return RESOURCE_FORMAT_UNDEFINED;
    }
    virtual resource_extent get_extent() const noexcept override { return {0, 0}; }
    virtual std::size_t get_buffer_size() const noexcept override { return 0; }
};

class vk_image : public vk_resource
{
public:
    vk_image(vk_rhi* rhi);

    virtual VkImageView get_image_view() const noexcept = 0;

protected:
    VkImageView create_image_view(VkImageViewCreateInfo* info);
    void destroy_image_view(VkImageView image_view);

    VkDevice get_device() const noexcept;

private:
    vk_rhi* m_rhi;
};

class vk_swapchain_image : public vk_image
{
public:
    vk_swapchain_image(VkImage image, VkFormat format, const VkExtent2D& extent, vk_rhi* rhi);
    vk_swapchain_image(const vk_swapchain_image&) = delete;
    vk_swapchain_image(vk_swapchain_image&& other) noexcept;
    virtual ~vk_swapchain_image();

    virtual VkImageView get_image_view() const noexcept override;
    virtual resource_format get_format() const noexcept override;
    virtual resource_extent get_extent() const noexcept override;

    vk_swapchain_image& operator=(const vk_swapchain_image&) = delete;
    vk_swapchain_image& operator=(vk_swapchain_image&& other) noexcept;

protected:
    VkImage m_image;
    VkImageView m_image_view;

    VkFormat m_format;
    VkExtent2D m_extent;
};
} // namespace violet::vk