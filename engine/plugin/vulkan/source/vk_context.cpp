#include "vk_context.hpp"
#include "vk_command.hpp"
#include "vk_descriptor_pool.hpp"
#include "vk_pipeline.hpp"
#include "vk_renderer.hpp"
#include <iostream>
#include <vector>

namespace ash::graphics::vk
{
vk_frame_counter::vk_frame_counter() noexcept : m_frame_counter(0), m_frame_resousrce_count(0)
{
}

vk_frame_counter& vk_frame_counter::instance() noexcept
{
    static vk_frame_counter instance;
    return instance;
}

void vk_frame_counter::initialize(
    std::size_t frame_counter,
    std::size_t frame_resousrce_count) noexcept
{
    instance().m_frame_counter = frame_counter;
    instance().m_frame_resousrce_count = frame_resousrce_count;
}

void vk_frame_counter::tick() noexcept
{
    ++instance().m_frame_counter;
}

static const std::vector<const char*> validation_layers = {"VK_LAYER_KHRONOS_validation"};

#ifndef NODEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data)
{
    std::cout << "validation layer: " << data->pMessage << std::endl;
    return VK_FALSE;
}
#endif

vk_context::vk_context() : m_physical_device(VK_NULL_HANDLE)
{
}

vk_context& vk_context::instance()
{
    static vk_context instance;
    return instance;
}

bool vk_context::on_initialize(const renderer_desc& config)
{
    create_instance();
    create_surface(config.window_handle);
    create_device();
    create_swap_chain(config.width, config.height);
    create_command_queue();
    create_semaphore();
    create_descriptor_pool();

    vk_frame_counter::initialize(0, 3);

    return true;
}

void vk_context::on_deinitialize()
{
    m_swap_chain = nullptr;

#ifndef NODEBUG
    auto destroy_debug_func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
    ASH_VK_ASSERT(destroy_debug_func != nullptr);
    destroy_debug_func(m_instance, m_debug_callback, nullptr);
#endif
    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

std::size_t vk_context::on_begin_frame()
{
    vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_fence);

    vkAcquireNextImageKHR(
        m_device,
        vk_context::swap_chain().swap_chain(),
        UINT64_MAX,
        m_image_available,
        VK_NULL_HANDLE,
        &m_image_index);

    return m_image_index;
}

void vk_context::on_end_frame()
{
    m_graphics_queue->execute_batch();

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_render_finished;

    VkSwapchainKHR swap_chains[] = {vk_context::swap_chain().swap_chain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swap_chains;
    presentInfo.pImageIndices = &m_image_index;

    vkQueuePresentKHR(m_present_queue->queue(), &presentInfo);

    vk_frame_counter::tick();
    m_graphics_queue->switch_frame_resources();
}

void vk_context::create_instance()
{
    // Create vulkan instance.
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "ASH_APPLICATION";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "AshEngine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;

    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef ASH_VK_WIN32
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

#ifndef NODEBUG
    // Initialize validation layer.
    if (!check_validation_layer())
        throw vk_exception("No verification layer found");

    instance_info.enabledLayerCount = static_cast<std::uint32_t>(validation_layers.size());
    instance_info.ppEnabledLayerNames = validation_layers.data();
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#else
    instance_info.enabledLayerCount = 0;
#endif

    instance_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    instance_info.ppEnabledExtensionNames = extensions.data();

    throw_if_failed(vkCreateInstance(&instance_info, nullptr, &m_instance));

#ifndef NODEBUG
    // Enable validation layer.
    VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = debug_callback;
    debug_info.pUserData = nullptr;

    auto create_debug_func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
    ASH_VK_ASSERT(create_debug_func != nullptr);

    throw_if_failed(create_debug_func(m_instance, &debug_info, nullptr, &m_debug_callback));
#endif
}

void vk_context::create_surface(void* window_handle)
{
#ifdef ASH_VK_WIN32
    VkWin32SurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hwnd = static_cast<HWND>(window_handle);
    surface_info.hinstance = GetModuleHandle(nullptr);

    auto create_surface_func = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
        vkGetInstanceProcAddr(m_instance, "vkCreateWin32SurfaceKHR"));
    ASH_VK_ASSERT(create_surface_func != nullptr);
    throw_if_failed(create_surface_func(m_instance, &surface_info, nullptr, &m_surface));
#endif
}

void vk_context::create_device()
{
    // Pick physical device.
    std::uint32_t device_count = 0;
    throw_if_failed(vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr));
    std::vector<VkPhysicalDevice> devices(device_count);
    throw_if_failed(vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data()));

    // Device extensions.
    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    for (auto device : devices)
    {
        if (check_device_extension_support(device, extensions) &&
            check_device_swap_chain_support(device))
            m_physical_device = device;
    }

    if (m_physical_device == VK_NULL_HANDLE)
        throw vk_exception("Cannot find Vulkan device.");

    // Device queue.
    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueCount = 1;
    float priority = 1.0;
    queue_info.pQueuePriorities = &priority;

    std::uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        m_physical_device,
        &queue_family_count,
        queue_families.data());
    for (std::size_t i = 0; i < queue_families.size(); ++i)
    {
        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            m_physical_device,
            static_cast<std::uint32_t>(i),
            m_surface,
            &present_support);

        if (queue_families[i].queueCount > 0 &&
            (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present_support)
        {
            m_queue_index.graphics = static_cast<std::uint32_t>(i);
            m_queue_index.present = static_cast<std::uint32_t>(i);
            break;
        }
    }
    queue_info.queueFamilyIndex = m_queue_index.graphics;

    // Device feature.
    VkPhysicalDeviceFeatures device_features = {};

    // Create logic device.
    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.queueCreateInfoCount = 1;
    device_info.pEnabledFeatures = &device_features;
    device_info.ppEnabledExtensionNames = extensions.data();
    device_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());

    throw_if_failed(vkCreateDevice(m_physical_device, &device_info, nullptr, &m_device));
}

void vk_context::create_swap_chain(std::uint32_t width, std::uint32_t height)
{
    m_swap_chain = std::make_unique<vk_swap_chain>(m_surface, width, height);
}

void vk_context::create_command_queue()
{
    VkQueue graphics_queue;
    vkGetDeviceQueue(m_device, m_queue_index.graphics, 0, &graphics_queue);
    m_graphics_queue = std::make_unique<vk_command_queue>(graphics_queue, 3);

    VkQueue present_queue;
    vkGetDeviceQueue(m_device, m_queue_index.present, 0, &present_queue);
    m_present_queue = std::make_unique<vk_command_queue>(present_queue, 3);
}

void vk_context::create_semaphore()
{
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    throw_if_failed(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_image_available));
    throw_if_failed(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_render_finished));

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    throw_if_failed(vkCreateFence(m_device, &fence_info, nullptr, &m_fence));
}

void vk_context::create_descriptor_pool()
{
    m_descriptor_pool = std::make_unique<vk_descriptor_pool>();
}

bool vk_context::check_validation_layer()
{
    std::uint32_t layer_count = 0;

    throw_if_failed(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
    std::vector<VkLayerProperties> layers(layer_count);
    throw_if_failed(vkEnumerateInstanceLayerProperties(&layer_count, layers.data()));

    for (const char* name : validation_layers)
    {
        bool found = false;
        for (auto& layer : layers)
        {
            if (std::strcmp(name, layer.layerName) == 0)
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

bool vk_context::check_device_extension_support(
    VkPhysicalDevice device,
    const std::vector<const char*>& extensions)
{
    std::uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> device_extension(extension_count);
    vkEnumerateDeviceExtensionProperties(
        device,
        nullptr,
        &extension_count,
        device_extension.data());

    for (const char* extension : extensions)
    {
        bool flag = false;
        for (auto& support : device_extension)
        {
            if (std::strcmp(extension, support.extensionName) == 0)
            {
                flag = true;
                break;
            }
        }

        if (flag == false)
            return false;
    }

    return true;
}

bool vk_context::check_device_swap_chain_support(VkPhysicalDevice device)
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &capabilities);

    std::uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);

    std::uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, nullptr);

    return format_count != 0 && present_mode_count != 0;
}
} // namespace ash::graphics::vk