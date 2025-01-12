#include "vk_swapchain.hpp"
#include "vk_command.hpp"
#include "vk_util.hpp"
#include <algorithm>

namespace violet::vk
{
vk_swapchain_image::vk_swapchain_image(
    VkImage image,
    VkFormat format,
    const VkExtent2D& extent,
    vk_context* context)
    : m_context(context)
{
    VkImageViewCreateInfo image_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    vkCreateImageView(m_context->get_device(), &image_view_info, nullptr, &m_image_view);

    m_image = image;
    m_format = vk_util::map_format(format);
    m_extent = {extent.width, extent.height};
    m_hash = hash::city_hash_64(&m_image_view, sizeof(VkImageView));
}

vk_swapchain_image::~vk_swapchain_image()
{
    vkDestroyImageView(m_context->get_device(), m_image_view, nullptr);
}

vk_swapchain::vk_swapchain(const rhi_swapchain_desc& desc, vk_context* context)
    : m_swapchain(VK_NULL_HANDLE),
      m_swapchain_image_index(0),
      m_flags(desc.flags),
      m_context(context)
{
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hwnd = static_cast<HWND>(desc.window_handle);
    surface_info.hinstance = GetModuleHandle(nullptr);

    vk_check(
        vkCreateWin32SurfaceKHR(m_context->get_instance(), &surface_info, nullptr, &m_surface));
#else
    throw std::runtime_error("Unsupported platform");
#endif

    for (std::size_t i = 0; i < m_context->get_frame_resource_count(); ++i)
    {
        m_available_semaphores.emplace_back(std::make_unique<vk_fence>(false, m_context));
        m_present_semaphores.emplace_back(std::make_unique<vk_fence>(false, m_context));
    }

    update();
}

vk_swapchain::~vk_swapchain()
{
    m_swapchain_images.clear();
    vkDestroySwapchainKHR(m_context->get_device(), m_swapchain, nullptr);
    vkDestroySurfaceKHR(m_context->get_instance(), m_surface, nullptr);
}

rhi_fence* vk_swapchain::acquire_texture()
{
    auto& semaphore = m_available_semaphores[m_context->get_frame_resource_index()];

    if (m_resized)
    {
        update();
        m_resized = false;
    }

    VkResult result = vkAcquireNextImageKHR(
        m_context->get_device(),
        m_swapchain,
        UINT64_MAX,
        semaphore->get_semaphore(),
        VK_NULL_HANDLE,
        &m_swapchain_image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        update();

        result = vkAcquireNextImageKHR(
            m_context->get_device(),
            m_swapchain,
            UINT64_MAX,
            semaphore->get_semaphore(),
            VK_NULL_HANDLE,
            &m_swapchain_image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return nullptr;
        }

        vk_check(result);
    }
    else
    {
        vk_check(result);
    }

    return semaphore.get();
}

rhi_fence* vk_swapchain::get_present_fence() const
{
    return m_present_semaphores[m_context->get_frame_resource_index()].get();
}

void vk_swapchain::present()
{
    auto* queue = m_context->get_present_queue();
    queue->present(
        m_swapchain,
        m_swapchain_image_index,
        m_present_semaphores[m_context->get_frame_resource_index()]->get_semaphore());
}

rhi_texture* vk_swapchain::get_texture()
{
    return m_swapchain_images[m_swapchain_image_index].get();
}

void vk_swapchain::update()
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        m_context->get_physical_device(),
        m_surface,
        &capabilities);

    if (m_swapchain != VK_NULL_HANDLE)
    {
        if (capabilities.currentExtent.width == 0 || capabilities.currentExtent.height == 0)
        {
            return;
        }

        vk_check(vkDeviceWaitIdle(m_context->get_device()));

        m_swapchain_images.clear();
        vkDestroySwapchainKHR(m_context->get_device(), m_swapchain, nullptr);
    }

    std::uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_context->get_physical_device(),
        m_surface,
        &format_count,
        nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_context->get_physical_device(),
        m_surface,
        &format_count,
        formats.data());

    std::uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_context->get_physical_device(),
        m_surface,
        &present_mode_count,
        nullptr);
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_context->get_physical_device(),
        m_surface,
        &present_mode_count,
        present_modes.data());

    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_surface,
        .imageFormat = formats[0].format,
        .imageColorSpace = formats[0].colorSpace,
        .imageArrayLayers = 1,
        .imageUsage = vk_util::map_image_usage_flags(m_flags),
    };

    if (capabilities.maxImageCount > 0 && swapchain_info.minImageCount > capabilities.maxImageCount)
    {
        swapchain_info.minImageCount = capabilities.maxImageCount;
    }
    else
    {
        swapchain_info.minImageCount = capabilities.minImageCount + 1;
    }

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
        swapchain_info.imageExtent.width = std::clamp(
            capabilities.currentExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        swapchain_info.imageExtent.height = std::clamp(
            capabilities.currentExtent.height,
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

    m_context->setup_present_queue(m_surface);

    std::vector<std::uint32_t> queue_family_indexes{
        m_context->get_graphics_queue()->get_family_index(),
        m_context->get_present_queue()->get_family_index()};
    if (queue_family_indexes[0] == queue_family_indexes[1])
    {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount =
            static_cast<std::uint32_t>(queue_family_indexes.size());
        swapchain_info.pQueueFamilyIndices = queue_family_indexes.data();
    }

    swapchain_info.preTransform = capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;

    vk_check(vkCreateSwapchainKHR(m_context->get_device(), &swapchain_info, nullptr, &m_swapchain));

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