#include "vk_rhi.hpp"
#include <algorithm>
#include <iostream>

namespace violet::vk
{
namespace
{
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data)
{
    std::cout << "validation layer: " << data->pMessage << std::endl;
    return VK_FALSE;
}
} // namespace

vk_rhi::vk_rhi() noexcept
    : m_instance(VK_NULL_HANDLE),
      m_physical_device(VK_NULL_HANDLE),
      m_device(VK_NULL_HANDLE),
      m_surface(VK_NULL_HANDLE),
      m_swapchain(VK_NULL_HANDLE),
      m_graphics_queue(VK_NULL_HANDLE),
      m_present_queue(VK_NULL_HANDLE),
      m_queue_family_indices{},
      m_frame_count(0),
      m_frame_resource_count(0),
      m_frame_resource_index(0)
{
#ifndef NDEUBG
    m_debug_messenger = VK_NULL_HANDLE;
#endif
}

vk_rhi::~vk_rhi()
{
    std::cout << "delete rhi" << std::endl;

    m_swapchain_images.clear();

#ifndef NDEBUG
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    vkDestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
#endif

    vkDestroyInstance(m_instance, nullptr);
}

bool vk_rhi::initialize(const rhi_desc& desc)
{
    m_frame_resource_count = desc.frame_resource;

    std::vector<const char*> instance_desired_layers;
    std::vector<const char*> instance_desired_extensions = {VK_KHR_SURFACE_EXTENSION_NAME};

#ifdef _WIN32
    instance_desired_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

#ifndef NDEBUG
    instance_desired_layers.push_back("VK_LAYER_KHRONOS_validation");
    instance_desired_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    if (!initialize_instance(instance_desired_layers, instance_desired_extensions))
        return false;

    std::vector<const char*> device_desired_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    if (!initialize_physical_device(device_desired_extensions))
        return false;

#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hwnd = static_cast<HWND>(desc.window_handle);
    surface_info.hinstance = GetModuleHandle(nullptr);

    auto vkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
        vkGetInstanceProcAddr(m_instance, "vkCreateWin32SurfaceKHR"));
    throw_if_failed(vkCreateWin32SurfaceKHR(m_instance, &surface_info, nullptr, &m_surface));
#else
    throw std::runtime_error("Unsupported platform");
#endif

    initialize_logic_device(device_desired_extensions);
    m_framebuffer_cache = std::make_unique<vk_framebuffer_cache>(m_device);

    initialize_swapchain(desc);

    return true;
}

void vk_rhi::present()
{
    ++m_frame_count;
    m_frame_resource_index = m_frame_count % 3;
}

rhi_resource* vk_rhi::get_back_buffer()
{
    return &m_swapchain_images[m_frame_count % m_swapchain_images.size()];
}

rhi_render_pipeline* vk_rhi::make_render_pipeline(const render_pipeline_desc& desc)
{
    return nullptr;
}

rhi_pipeline_parameter* vk_rhi::make_pipeline_parameter(const pipeline_parameter_desc& desc)
{
    return nullptr;
}

rhi_resource* vk_rhi::make_vertex_buffer(const vertex_buffer_desc& desc)
{
    return nullptr;
}

rhi_resource* vk_rhi::make_index_buffer(const index_buffer_desc& desc)
{
    return nullptr;
}

rhi_resource* vk_rhi::make_texture(
    const std::uint8_t* data,
    std::uint32_t width,
    std::uint32_t height,
    resource_format format)
{
    return nullptr;
}

rhi_resource* vk_rhi::make_texture(const char* file)
{
    return nullptr;
}

rhi_resource* vk_rhi::make_texture_cube(
    const char* left,
    const char* right,
    const char* top,
    const char* bottom,
    const char* front,
    const char* back)
{
    return nullptr;
}

rhi_resource* vk_rhi::make_shadow_map(const shadow_map_desc& desc)
{
    return nullptr;
}

rhi_resource* vk_rhi::make_render_target(const render_target_desc& desc)
{
    return nullptr;
}

rhi_resource* vk_rhi::make_depth_stencil_buffer(const depth_stencil_buffer_desc& desc)
{
    return nullptr;
}

bool vk_rhi::initialize_instance(
    const std::vector<const char*>& desired_layers,
    const std::vector<const char*>& desired_extensions)
{
    std::uint32_t available_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);
    std::vector<VkLayerProperties> available_layers(available_layer_count);
    vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers.data());

    for (const char* layer : desired_layers)
    {
        bool found = false;

        for (const VkLayerProperties& available : available_layers)
        {
            if (std::strcmp(available.layerName, layer) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
            return false;
    }

    std::uint32_t available_extension_count = 0;
    throw_if_failed(
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr));
    std::vector<VkExtensionProperties> available_extensions(available_extension_count);
    throw_if_failed(vkEnumerateInstanceExtensionProperties(
        nullptr,
        &available_extension_count,
        available_extensions.data()));
    if (!check_extension_support(desired_extensions, available_extensions))
        return false;

    VkApplicationInfo app_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "violet app",
        VK_MAKE_VERSION(1, 0, 0),
        "violet engine",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_0};

    VkInstanceCreateInfo instance_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        0,
        &app_info,
        static_cast<std::uint32_t>(desired_layers.size()),
        desired_layers.size() > 0 ? desired_layers.data() : nullptr,
        static_cast<std::uint32_t>(desired_extensions.size()),
        desired_extensions.size() > 0 ? desired_extensions.data() : nullptr};

#ifndef NDEBUG
    VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    debug_info.pfnUserCallback = debug_callback;
    debug_info.pUserData = nullptr;

    instance_info.pNext = &debug_info;
#endif

    throw_if_failed(vkCreateInstance(&instance_info, nullptr, &m_instance));
    if (m_instance == VK_NULL_HANDLE)
        return false;

#ifndef NDEBUG
    auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));

    throw_if_failed(
        vkCreateDebugUtilsMessengerEXT(m_instance, &debug_info, nullptr, &m_debug_messenger));
#endif

    return true;
}

bool vk_rhi::initialize_physical_device(const std::vector<const char*>& desired_extensions)
{
    std::uint32_t devices_count = 0;
    throw_if_failed(vkEnumeratePhysicalDevices(m_instance, &devices_count, nullptr));
    std::vector<VkPhysicalDevice> available_devices(devices_count);
    throw_if_failed(
        vkEnumeratePhysicalDevices(m_instance, &devices_count, available_devices.data()));

    std::uint32_t physical_device_score = 0;
    for (VkPhysicalDevice device : available_devices)
    {
        std::uint32_t available_extension_count = 0;
        throw_if_failed(vkEnumerateDeviceExtensionProperties(
            device,
            nullptr,
            &available_extension_count,
            nullptr));
        std::vector<VkExtensionProperties> available_extensions(available_extension_count);
        throw_if_failed(vkEnumerateDeviceExtensionProperties(
            device,
            nullptr,
            &available_extension_count,
            available_extensions.data()));
        if (!check_extension_support(desired_extensions, available_extensions))
            continue;

        std::uint32_t score = 0;

        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(device, &properties);
        std::cout << properties.deviceName << std::endl;

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            score += 1000;

        score += properties.limits.maxImageDimension2D;

        VkPhysicalDeviceFeatures features = {};
        vkGetPhysicalDeviceFeatures(device, &features);

        if (physical_device_score < score)
        {
            m_physical_device = device;
            physical_device_score = score;
        }
    }

    if (m_physical_device == VK_NULL_HANDLE)
        return false;

    return true;
}

void vk_rhi::initialize_logic_device(const std::vector<const char*>& enabled_extensions)
{
    std::uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        m_physical_device,
        &queue_family_count,
        queue_families.data());

    m_queue_family_indices = {};
    for (std::uint32_t i = 0; i < queue_families.size(); ++i)
    {
        if (queue_families[i].queueCount == 0)
            continue;

        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            m_queue_family_indices.graphics = i;

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, i, m_surface, &present_support);
        if (present_support)
            m_queue_family_indices.present = i;

        if (m_queue_family_indices.graphics != 0 && m_queue_family_indices.present != 0)
            break;
    }

    std::vector<std::uint32_t> queue_indices = {
        m_queue_family_indices.graphics,
        m_queue_family_indices.present};
    std::vector<VkDeviceQueueCreateInfo> queue_infos = {};
    float queue_priority = 1.0;
    for (std::uint32_t index : queue_indices)
    {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;

        queue_infos.push_back(queue_info);
    }
    VkPhysicalDeviceFeatures desired_features = {};

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pQueueCreateInfos = queue_infos.data();
    device_info.queueCreateInfoCount = static_cast<std::uint32_t>(queue_infos.size());
    device_info.ppEnabledExtensionNames = enabled_extensions.data();
    device_info.enabledExtensionCount = static_cast<std::uint32_t>(enabled_extensions.size());
    device_info.pEnabledFeatures = &desired_features;

    throw_if_failed(vkCreateDevice(m_physical_device, &device_info, nullptr, &m_device));

    vkGetDeviceQueue(m_device, m_queue_family_indices.graphics, 0, &m_graphics_queue);
    vkGetDeviceQueue(m_device, m_queue_family_indices.present, 0, &m_present_queue);
}

void vk_rhi::initialize_swapchain(const rhi_desc& desc)
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &capabilities);

    std::uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_physical_device,
        m_surface,
        &format_count,
        formats.data());

    std::uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_physical_device,
        m_surface,
        &present_mode_count,
        nullptr);
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_physical_device,
        m_surface,
        &present_mode_count,
        present_modes.data());

    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = m_surface;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    swapchain_info.minImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && swapchain_info.minImageCount > capabilities.maxImageCount)
        swapchain_info.minImageCount = capabilities.maxImageCount;

    swapchain_info.imageFormat = formats[0].format;
    swapchain_info.imageColorSpace = formats[0].colorSpace;
    for (auto& format : formats)
    {
        if (format.format == VK_FORMAT_R8G8B8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            swapchain_info.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
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
            desc.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        swapchain_info.imageExtent.height = std::clamp(
            desc.height,
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

    std::vector<std::uint32_t> queue_family_indices = {
        m_queue_family_indices.graphics,
        m_queue_family_indices.present};
    if (m_queue_family_indices.graphics == m_queue_family_indices.present)
    {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;
    }
    else
    {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = queue_family_indices.data();
    }

    swapchain_info.preTransform = capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;

    throw_if_failed(vkCreateSwapchainKHR(m_device, &swapchain_info, nullptr, &m_swapchain));

    std::uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchain_image_count, nullptr);
    std::vector<VkImage> swapchain_images(swapchain_image_count);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchain_image_count, swapchain_images.data());

    for (VkImage swapchain_image : swapchain_images)
    {
        m_swapchain_images.emplace_back(
            swapchain_image,
            swapchain_info.imageFormat,
            swapchain_info.imageExtent,
            this);
    }
}

bool vk_rhi::check_extension_support(
    const std::vector<const char*>& desired_extensions,
    const std::vector<VkExtensionProperties>& available_extensions) const
{
    for (const char* extension : desired_extensions)
    {
        bool found = false;
        for (const VkExtensionProperties& available : available_extensions)
        {
            if (std::strcmp(extension, available.extensionName) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
            return false;
    }

    return true;
}
} // namespace violet::vk

extern "C"
{
    PLUGIN_API violet::plugin_info get_plugin_info()
    {
        violet::plugin_info info = {};

        char name[] = "graphics-vk";
        memcpy(info.name, name, sizeof(name));

        info.version.major = 1;
        info.version.minor = 0;

        return info;
    }

    PLUGIN_API violet::rhi_context* make_rhi()
    {
        return new violet::vk::vk_rhi();
    }

    PLUGIN_API void destroy_rhi(violet::rhi_context* rhi)
    {
        delete rhi;
    }
}