#include "vk_rhi.hpp"
#include "vk_command.hpp"
#include "vk_framebuffer.hpp"
#include "vk_pipeline.hpp"
#include "vk_render_pass.hpp"
#include "vk_sync.hpp"
#include <algorithm>
#include <iostream>
#include <set>

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
      m_swapchain_image_index(0),
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

    vkDeviceWaitIdle(m_device);

    m_graphics_queue = nullptr;
    m_present_queue = nullptr;
    m_swapchain_images.clear();

    for (vk_frame_resource& frame_resource : m_frame_resources)
        frame_resource.execute_delay_tasks();
    m_frame_resources.clear();

    vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);

#ifndef NDEBUG
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    vkDestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
#endif

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

bool vk_rhi::initialize(const rhi_desc& desc)
{
    m_frame_resource_count = desc.frame_resource_count;

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
    throw vk_exception("Unsupported platform");
#endif

    initialize_logic_device(device_desired_extensions);
    initialize_swapchain(desc.width, desc.height);
    initialize_frame_resources(desc.frame_resource_count);
    initialize_descriptor_pool();

    return true;
}

rhi_render_command* vk_rhi::allocate_command()
{
    return m_graphics_queue->allocate_command();
}

void vk_rhi::execute(
    rhi_render_command* const* commands,
    std::size_t command_count,
    rhi_semaphore* const* signal_semaphores,
    std::size_t signal_semaphore_count,
    rhi_semaphore* const* wait_semaphores,
    std::size_t wait_semaphore_count,
    rhi_fence* fence)
{
    m_graphics_queue->execute(
        commands,
        command_count,
        signal_semaphores,
        signal_semaphore_count,
        wait_semaphores,
        wait_semaphore_count,
        fence);
}

void vk_rhi::begin_frame()
{
    vk_frame_resource& frame_resource = m_frame_resources[m_frame_resource_index];

    VkFence fences[] = {frame_resource.in_flight_fence->get_fence()};

    vkWaitForFences(m_device, 1, fences, VK_TRUE, UINT64_MAX);

    vkAcquireNextImageKHR(
        m_device,
        m_swapchain,
        UINT64_MAX,
        frame_resource.image_available_semaphore->get_semaphore(),
        VK_NULL_HANDLE,
        &m_swapchain_image_index);

    vkResetFences(m_device, 1, fences);

    frame_resource.execute_delay_tasks();

    m_graphics_queue->begin_frame();
}

void vk_rhi::end_frame()
{
    ++m_frame_count;
    m_frame_resource_index = m_frame_count % m_frame_resource_count;
}

void vk_rhi::present(rhi_semaphore* const* wait_semaphores, std::size_t wait_semaphore_count)
{
    m_present_queue
        ->present(m_swapchain, m_swapchain_image_index, wait_semaphores, wait_semaphore_count);
}

void vk_rhi::resize(std::uint32_t width, std::uint32_t height)
{
    throw_if_failed(vkDeviceWaitIdle(m_device));

    m_swapchain_images.clear();
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    initialize_swapchain(width, height);
}

rhi_resource* vk_rhi::get_back_buffer()
{
    return m_swapchain_images[m_swapchain_image_index].get();
}

rhi_fence* vk_rhi::get_in_flight_fence()
{
    return m_frame_resources[m_frame_resource_index].in_flight_fence.get();
}

rhi_semaphore* vk_rhi::get_image_available_semaphore()
{
    return m_frame_resources[m_frame_resource_index].image_available_semaphore.get();
}

rhi_render_pass* vk_rhi::create_render_pass(const rhi_render_pass_desc& desc)
{
    return new vk_render_pass(desc, this);
}

void vk_rhi::destroy_render_pass(rhi_render_pass* render_pass)
{
    delay_delete(render_pass);
}

rhi_render_pipeline* vk_rhi::create_render_pipeline(const rhi_render_pipeline_desc& desc)
{
    return new vk_render_pipeline(desc, VkExtent2D{128, 128}, this);
}

void vk_rhi::destroy_render_pipeline(rhi_render_pipeline* render_pipeline)
{
    delay_delete(render_pipeline);
}

rhi_pipeline_parameter_layout* vk_rhi::create_pipeline_parameter_layout(
    const rhi_pipeline_parameter_layout_desc& desc)
{
    return new vk_pipeline_parameter_layout(desc, this);
}

void vk_rhi::destroy_pipeline_parameter_layout(
    rhi_pipeline_parameter_layout* pipeline_parameter_layout)
{
    delete pipeline_parameter_layout;
}

rhi_pipeline_parameter* vk_rhi::create_pipeline_parameter(rhi_pipeline_parameter_layout* layout)
{
    return new vk_pipeline_parameter(static_cast<vk_pipeline_parameter_layout*>(layout), this);
}

void vk_rhi::destroy_pipeline_parameter(rhi_pipeline_parameter* pipeline_parameter)
{
    delay_delete(pipeline_parameter);
}

rhi_framebuffer* vk_rhi::create_framebuffer(const rhi_framebuffer_desc& desc)
{
    return new vk_framebuffer(desc, this);
}

void vk_rhi::destroy_framebuffer(rhi_framebuffer* framebuffer)
{
    delay_delete(framebuffer);
}

rhi_resource* vk_rhi::create_vertex_buffer(const rhi_vertex_buffer_desc& desc)
{
    return new vk_vertex_buffer(desc, this);
}

void vk_rhi::destroy_vertex_buffer(rhi_resource* vertex_buffer)
{
    delay_delete(vertex_buffer);
}

rhi_resource* vk_rhi::create_index_buffer(const rhi_index_buffer_desc& desc)
{
    return new vk_index_buffer(desc, this);
}

void vk_rhi::destroy_index_buffer(rhi_resource* index_buffer)
{
    delay_delete(index_buffer);
}

rhi_sampler* vk_rhi::create_sampler(const rhi_sampler_desc& desc)
{
    return new vk_sampler(desc, this);
}

void vk_rhi::destroy_sampler(rhi_sampler* sampler)
{
    delay_delete(sampler);
}

rhi_resource* vk_rhi::create_texture(
    const std::uint8_t* data,
    std::uint32_t width,
    std::uint32_t height,
    rhi_resource_format format)
{
    return nullptr;
}

rhi_resource* vk_rhi::create_texture(const char* file)
{
    return new vk_texture(file, this);
}

void vk_rhi::destroy_texture(rhi_resource* texture)
{
    delay_delete(texture);
}

rhi_resource* vk_rhi::create_texture_cube(
    const char* left,
    const char* right,
    const char* top,
    const char* bottom,
    const char* front,
    const char* back)
{
    return nullptr;
}

rhi_resource* vk_rhi::create_shadow_map(const rhi_shadow_map_desc& desc)
{
    return nullptr;
}

rhi_resource* vk_rhi::create_render_target(const rhi_render_target_desc& desc)
{
    return nullptr;
}

rhi_resource* vk_rhi::create_depth_stencil_buffer(const rhi_depth_stencil_buffer_desc& desc)
{
    return nullptr;
}

rhi_fence* vk_rhi::create_fence(bool signaled)
{
    return new vk_fence(signaled, this);
}

void vk_rhi::destroy_fence(rhi_fence* fence)
{
    delay_delete(fence);
}

rhi_semaphore* vk_rhi::create_semaphore()
{
    return new vk_semaphore(this);
}

void vk_rhi::destroy_semaphore(rhi_semaphore* semaphore)
{
    delay_delete(semaphore);
}

VkDescriptorSet vk_rhi::allocate_descriptor_set(VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = m_descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;

    VkDescriptorSet result;
    throw_if_failed(vkAllocateDescriptorSets(m_device, &allocate_info, &result));

    return result;
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
        if (features.samplerAnisotropy == VK_FALSE)
            continue;

        if (physical_device_score < score)
        {
            m_physical_device = device;
            m_physical_device_properties = properties;
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

    m_queue_family_indices = {100, 100};
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

        if (m_queue_family_indices.graphics != 100 && m_queue_family_indices.present != 100)
            break;
    }

    std::set<std::uint32_t> queue_indices = {
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
    VkPhysicalDeviceFeatures enabled_features = {};
    enabled_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pQueueCreateInfos = queue_infos.data();
    device_info.queueCreateInfoCount = static_cast<std::uint32_t>(queue_infos.size());
    device_info.ppEnabledExtensionNames = enabled_extensions.data();
    device_info.enabledExtensionCount = static_cast<std::uint32_t>(enabled_extensions.size());
    device_info.pEnabledFeatures = &enabled_features;

    throw_if_failed(vkCreateDevice(m_physical_device, &device_info, nullptr, &m_device));

    m_graphics_queue = std::make_unique<vk_graphics_queue>(m_queue_family_indices.graphics, this);
    m_present_queue = std::make_unique<vk_present_queue>(m_queue_family_indices.present, this);
}

void vk_rhi::initialize_swapchain(std::uint32_t width, std::uint32_t height)
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

    std::vector<std::uint32_t> queue_family_indices = {
        m_queue_family_indices.graphics,
        m_queue_family_indices.present};
    if (m_queue_family_indices.graphics == m_queue_family_indices.present)
    {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
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
        m_swapchain_images.push_back(std::make_unique<vk_swapchain_image>(
            swapchain_image,
            swapchain_info.imageFormat,
            swapchain_info.imageExtent,
            this));
    }
}

void vk_rhi::initialize_frame_resources(std::size_t frame_resource_count)
{
    m_frame_resources.resize(frame_resource_count);

    for (vk_frame_resource& frame_resource : m_frame_resources)
    {
        frame_resource.image_available_semaphore = std::make_unique<vk_semaphore>(this);
        frame_resource.in_flight_fence = std::make_unique<vk_fence>(true, this);
    }
}

void vk_rhi::initialize_descriptor_pool()
{
    std::vector<VkDescriptorPoolSize> pool_size = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1024},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024}
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_size.size());
    pool_info.pPoolSizes = pool_size.data();
    pool_info.maxSets = 512;

    throw_if_failed(vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_descriptor_pool));
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

    PLUGIN_API violet::rhi_context* create_rhi()
    {
        return new violet::vk::vk_rhi();
    }

    PLUGIN_API void destroy_rhi(violet::rhi_context* rhi)
    {
        delete rhi;
    }
}