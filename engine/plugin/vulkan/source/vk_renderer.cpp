#include "vk_renderer.hpp"
#include "vk_command.hpp"
#include "vk_context.hpp"

namespace ash::graphics::vk
{
vk_swap_chain::vk_swap_chain(VkSurfaceKHR surface, std::uint32_t width, std::uint32_t height)
    : m_swap_chain(VK_NULL_HANDLE)
{
    m_surface_format = choose_surface_format(surface);
    m_depth_stencil_format = choose_depth_stencil_format();
    m_present_mode = choose_present_mode(surface);

    resize(width, height);
}

vk_swap_chain::~vk_swap_chain()
{
    destroy();
}

void vk_swap_chain::resize(std::uint32_t width, std::uint32_t height)
{
    destroy();

    auto physical_device = vk_context::physical_device();

    // Choose swap extent.
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physical_device,
        vk_context::surface(),
        &capabilities);
    m_extent = choose_swap_extent(capabilities, width, height);

    // Image count.
    std::uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
        image_count = capabilities.maxImageCount;

    // Create swap chain.
    VkSwapchainCreateInfoKHR swap_cahin_info = {};
    swap_cahin_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_cahin_info.surface = vk_context::surface();
    swap_cahin_info.minImageCount = image_count;
    swap_cahin_info.imageFormat = m_surface_format.format;
    swap_cahin_info.imageColorSpace = m_surface_format.colorSpace;
    swap_cahin_info.imageExtent = m_extent;
    swap_cahin_info.imageArrayLayers = 1;
    swap_cahin_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto queue_index = vk_context::queue_index();
    if (queue_index.graphics != queue_index.present)
    {
        swap_cahin_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swap_cahin_info.queueFamilyIndexCount = 2;

        std::uint32_t indices[] = {queue_index.graphics, queue_index.present};
        swap_cahin_info.pQueueFamilyIndices = indices;
    }
    else
    {
        swap_cahin_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swap_cahin_info.preTransform = capabilities.currentTransform;
    swap_cahin_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_cahin_info.presentMode = m_present_mode;
    swap_cahin_info.clipped = VK_TRUE;

    auto device = vk_context::device();
    throw_if_failed(vkCreateSwapchainKHR(device, &swap_cahin_info, nullptr, &m_swap_chain));

    // Get swap chain image.
    vkGetSwapchainImagesKHR(device, m_swap_chain, &image_count, nullptr);
    std::vector<VkImage> images(image_count);
    vkGetSwapchainImagesKHR(device, m_swap_chain, &image_count, images.data());

    // Create image view.
    for (VkImage image : images)
        m_back_buffers.emplace_back(image, m_surface_format.format, m_extent);
}

void vk_swap_chain::destroy()
{
    auto device = vk_context::device();
    vkDeviceWaitIdle(device);
    m_back_buffers.clear();

    if (m_swap_chain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(device, m_swap_chain, nullptr);
}

VkSurfaceFormatKHR vk_swap_chain::choose_surface_format(VkSurfaceKHR surface)
{
    auto physical_device = vk_context::physical_device();

    std::uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    ASH_VK_ASSERT(format_count != 0);

    std::vector<VkSurfaceFormatKHR> formats(format_count);
    if (!formats.empty())
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physical_device,
            surface,
            &format_count,
            formats.data());

    VkSurfaceFormatKHR result;
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        result.format = VK_FORMAT_R8G8B8A8_UNORM;
        result.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }
    else
    {
        result = formats[0];

        for (auto& format : formats)
        {
            if (format.format == VK_FORMAT_R8G8B8A8_UNORM &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                result.format = VK_FORMAT_R8G8B8A8_UNORM;
                result.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            }
        }
    }

    return result;
}

VkFormat vk_swap_chain::choose_depth_stencil_format()
{
    auto physical_device = vk_context::physical_device();

    std::vector<VkFormat> candidates = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (VkFormat format : candidates)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &properties);

        if ((properties.linearTilingFeatures & features) == features)
            return format;
        else if ((properties.optimalTilingFeatures & features) == features)
            return format;
    }

    throw vk_exception("Failed to find supported format.");
}

VkPresentModeKHR vk_swap_chain::choose_present_mode(VkSurfaceKHR surface)
{
    auto physical_device = vk_context::physical_device();

    std::uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device,
        surface,
        &present_mode_count,
        nullptr);
    ASH_VK_ASSERT(present_mode_count != 0);

    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    if (!present_modes.empty())
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physical_device,
            surface,
            &present_mode_count,
            present_modes.data());

    VkPresentModeKHR result = VK_PRESENT_MODE_FIFO_KHR;
    for (VkPresentModeKHR mode : present_modes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            result = VK_PRESENT_MODE_MAILBOX_KHR;
        else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            result = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    return result;
}

VkExtent2D vk_swap_chain::choose_swap_extent(
    const VkSurfaceCapabilitiesKHR& capabilities,
    std::uint32_t width,
    std::uint32_t height)
{
    VkExtent2D result;

    // Use (std::numeric_limits<std::uint32_t>::max)() avoid max being replaced by macros in
    // minwindef.h
    if (capabilities.currentExtent.width != (std::numeric_limits<std::uint32_t>::max)())
    {
        result = capabilities.currentExtent;
    }
    else
    {
        result = {width, height};

        result.width = std::clamp(
            result.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        result.height = std::clamp(
            result.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);
    }

    return result;
}

vk_renderer::vk_renderer(const renderer_desc& desc)
{
    vk_context::initialize(desc);
}

std::size_t vk_renderer::begin_frame()
{
    return vk_context::begin_frame();
}

void vk_renderer::end_frame()
{
    vk_context::end_frame();
}

render_command_interface* vk_renderer::allocate_command()
{
    return vk_context::graphics_queue().allocate_command();
}

void vk_renderer::execute(render_command_interface* command)
{
    vk_context::graphics_queue().execute(static_cast<vk_command*>(command));
}

resource_interface* vk_renderer::back_buffer(std::size_t index)
{
    return &vk_context::swap_chain().back_buffers()[index];
}

std::size_t vk_renderer::back_buffer_count()
{
    return vk_context::swap_chain().back_buffers().size();
}

void vk_renderer::resize(std::uint32_t width, std::uint32_t height)
{
    vk_context::swap_chain().resize(width, height);
}
} // namespace ash::graphics::vk