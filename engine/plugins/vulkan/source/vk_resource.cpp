#include "vk_resource.hpp"
#include "vk_rhi.hpp"
#include "vk_util.hpp"

namespace violet::vk
{
vk_image::vk_image(vk_rhi* rhi) : m_rhi(rhi)
{
}

VkImageView vk_image::create_image_view(VkImageViewCreateInfo* info)
{
    VkImageView image_view;
    vkCreateImageView(get_device(), info, nullptr, &image_view);
    return image_view;
}

void vk_image::destroy_image_view(VkImageView image_view)
{
    m_rhi->get_framebuffer_cache()->on_destroy_image(image_view);
    vkDestroyImageView(m_rhi->get_device(), image_view, nullptr);
}

VkDevice vk_image::get_device() const noexcept
{
    return m_rhi->get_device();
}

vk_swapchain_image::vk_swapchain_image(
    VkImage image,
    VkFormat format,
    const VkExtent2D& extent,
    vk_rhi* rhi)
    : vk_image(rhi),
      m_image(image),
      m_format(format),
      m_extent(extent)
{
    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;

    m_image_view = create_image_view(&image_view_info);
}

vk_swapchain_image::vk_swapchain_image(vk_swapchain_image&& other) noexcept
    : vk_image(std::move(other))
{
    m_image = other.m_image;
    m_image_view = other.m_image_view;
    m_format = other.m_format;
    m_extent = other.m_extent;

    other.m_image = VK_NULL_HANDLE;
    other.m_image_view = VK_NULL_HANDLE;
}

vk_swapchain_image::~vk_swapchain_image()
{
    destroy_image_view(m_image_view);
}

VkImageView vk_swapchain_image::get_image_view() const noexcept
{
    return m_image_view;
}

resource_format vk_swapchain_image::get_format() const noexcept
{
    return convert(m_format);
}

resource_extent vk_swapchain_image::get_extent() const noexcept
{
    return {m_extent.width, m_extent.height};
}

vk_swapchain_image& vk_swapchain_image::operator=(vk_swapchain_image&& other) noexcept
{
    vk_image::operator=(std::move(other));

    m_image = other.m_image;
    m_image_view = other.m_image_view;
    m_format = other.m_format;
    m_extent = other.m_extent;

    other.m_image = VK_NULL_HANDLE;
    other.m_image_view = VK_NULL_HANDLE;

    return *this;
}
} // namespace violet::vk