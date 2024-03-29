#include "vk_swapchain.hpp"
#include "vk_util.hpp"
#include <algorithm>

namespace violet::vk
{
vk_swapchain_image::vk_swapchain_image(
    VkImage image,
    VkFormat format,
    const VkExtent2D& extent,
    vk_context* context)
    : vk_image(context)
{
    VkImageView image_view;
    create_image_view(image, format, VK_IMAGE_ASPECT_COLOR_BIT, image_view);

    std::hash<void*> hasher;
    std::size_t hash = vk_util::hash_combine(hasher(image), hasher(image_view));
    hash = vk_util::hash_combine(hash, get_context()->get_frame_count());

    set_image_view(image_view);
    set_format(format);
    set_extent(extent);
    set_hash(hash);
    set_clear_value(VkClearColorValue{0.0f, 0.0f, 0.0f, 1.0f});
}

vk_swapchain_image::~vk_swapchain_image()
{
}

vk_swapchain::vk_swapchain(
    std::uint32_t width,
    std::uint32_t height,
    const std::vector<std::uint32_t>& queue_family_indices,
    vk_context* context)
    : m_swapchain(VK_NULL_HANDLE),
      m_swapchain_image_index(0),
      m_context(context)
{
    if (queue_family_indices[0] == queue_family_indices[1])
    {
        m_queue_family_indices.push_back(queue_family_indices[0]);
    }
    else
    {
        m_queue_family_indices.push_back(queue_family_indices[0]);
        m_queue_family_indices.push_back(queue_family_indices[1]);
    }

    resize(width, height);
}

vk_swapchain::~vk_swapchain()
{
    m_swapchain_images.clear();
    vkDestroySwapchainKHR(m_context->get_device(), m_swapchain, nullptr);
}

std::uint32_t vk_swapchain::acquire_next_image(VkSemaphore signal_semaphore)
{
    vk_check(vkAcquireNextImageKHR(
        m_context->get_device(),
        m_swapchain,
        UINT64_MAX,
        signal_semaphore,
        VK_NULL_HANDLE,
        &m_swapchain_image_index));

    return m_swapchain_image_index;
}

void vk_swapchain::resize(std::uint32_t width, std::uint32_t height)
{
    if (m_swapchain != VK_NULL_HANDLE)
    {
        m_swapchain_images.clear();
        vkDestroySwapchainKHR(m_context->get_device(), m_swapchain, nullptr);
    }

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        m_context->get_physical_device(),
        m_context->get_surface(),
        &capabilities);

    std::uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_context->get_physical_device(),
        m_context->get_surface(),
        &format_count,
        nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_context->get_physical_device(),
        m_context->get_surface(),
        &format_count,
        formats.data());

    std::uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_context->get_physical_device(),
        m_context->get_surface(),
        &present_mode_count,
        nullptr);
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_context->get_physical_device(),
        m_context->get_surface(),
        &present_mode_count,
        present_modes.data());

    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = m_context->get_surface();
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    swapchain_info.minImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && swapchain_info.minImageCount > capabilities.maxImageCount)
        swapchain_info.minImageCount = capabilities.maxImageCount;

    swapchain_info.imageFormat = formats[0].format;
    swapchain_info.imageColorSpace = formats[0].colorSpace;
    for (auto& format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            swapchain_info.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
            swapchain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            break;
        }
    }

    // Use (std::numeric_limits<std::uint32_t>::max)() avoid max being replaced by macros in
    // minwindef.h
    if (capabilities.currentExtent.width != (std::numeric_limits<std::uint32_t>::max)())
    {
        swapchain_info.imageExtent = capabilities.currentExtent;
    }
    else
    {
        swapchain_info.imageExtent.width =
            std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        swapchain_info.imageExtent.height = std::clamp(
            height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);
    }

    swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (VkPresentModeKHR present_mode : present_modes)
    {
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchain_info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    if (m_queue_family_indices.size() == 1)
    {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount =
            static_cast<std::uint32_t>(m_queue_family_indices.size());
        swapchain_info.pQueueFamilyIndices = m_queue_family_indices.data();
    }

    swapchain_info.preTransform = capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;

    vk_check(
        vkCreateSwapchainKHR(m_context->get_device(), &swapchain_info, nullptr, &m_swapchain));

    std::uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(m_context->get_device(), m_swapchain, &swapchain_image_count, nullptr);
    std::vector<VkImage> swapchain_images(swapchain_image_count);
    vkGetSwapchainImagesKHR(
        m_context->get_device(),
        m_swapchain,
        &swapchain_image_count,
        swapchain_images.data());

    for (VkImage swapchain_image : swapchain_images)
    {
        m_swapchain_images.push_back(std::make_unique<vk_swapchain_image>(
            swapchain_image,
            swapchain_info.imageFormat,
            swapchain_info.imageExtent,
            m_context));
    }
}
} // namespace violet::vk