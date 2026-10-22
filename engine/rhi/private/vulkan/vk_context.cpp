#include "vk_context.hpp"
#include "common/log.hpp"
#include "rhi/vulkan/rhi_vulkan.hpp"
#include "vk_bindless.hpp"
#include "vk_command.hpp"
#include "vk_framebuffer.hpp"
#include "vk_layout.hpp"
#include "vk_parameter.hpp"
#include "vk_staging_uploader.hpp"
#include <algorithm>
#include <set>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace violet::vk
{
namespace
{
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data)
{
    log::warn("[vulkan] validation: {}", data->pMessage);
    return VK_FALSE;
}

template <typename Functor>
void each_extension(std::span<rhi_extension*> extensions, Functor&& functor)
{
    for (auto& extension : extensions)
    {
        if (extension->is_enabled())
        {
            auto* vk = static_cast<vk_extension*>(extension);
            functor(vk);
        }
    }
}
} // namespace

vk_context::vk_context() noexcept = default;

vk_context::~vk_context()
{
    vkDeviceWaitIdle(m_device);

    m_graphics_queue = nullptr;
    m_present_queue = nullptr;
    m_layout_manager = nullptr;
    m_parameter_manager = nullptr;
    m_bindless_manager = nullptr;
    m_framebuffer_manager = nullptr;
    m_staging_uploader = nullptr;
    m_deletion_queue = nullptr;

    vkDestroyDescriptorPool(m_device, m_descriptor_pool, nullptr);

    vmaDestroyAllocator(m_vma_allocator);

#ifndef NDEBUG
    vkDestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
#endif

    vkDestroyDevice(m_device, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

bool vk_context::initialize(const rhi_desc& desc)
{
    vk_check(volkInitialize());

    m_frame_resource_count = desc.frame_resource_count;

    std::span<rhi_extension*> extensions(desc.extensions, desc.extension_count);
    vk_extension_registry registry;

    if (!initialize_instance(extensions, registry))
    {
        return false;
    }

    if (!initialize_physical_device(extensions, registry))
    {
        return false;
    }

    initialize_device(registry);
    initialize_vma(registry);
    initialize_descriptor_pool(true);

    m_deletion_queue = std::make_unique<vk_deletion_queue>(this);

    m_layout_manager = std::make_unique<vk_layout_manager>(this);
    m_parameter_manager = std::make_unique<vk_parameter_manager>(this);
    m_bindless_manager = std::make_unique<vk_bindless_manager>(this);
    m_framebuffer_manager = std::make_unique<vk_framebuffer_manager>(this);

    m_staging_uploader = std::make_unique<vk_staging_uploader>(10 * 1024 * 1024, this);

    return true;
}

void vk_context::begin_frame()
{
    m_deletion_queue->tick(m_frame_count);
    m_graphics_queue->begin_frame();

    m_parameter_manager->sync_parameter();
}

void vk_context::end_frame()
{
    ++m_frame_count;
    m_frame_resource_index = m_frame_count % m_frame_resource_count;
}

VkDescriptorSet vk_context::allocate_descriptor_set(VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    std::scoped_lock lock(m_mutex);

    VkDescriptorSet result;
    vk_check(vkAllocateDescriptorSets(m_device, &allocate_info, &result));

    return result;
}

void vk_context::free_descriptor_set(VkDescriptorSet descriptor_set)
{
    vkFreeDescriptorSets(m_device, m_descriptor_pool, 1, &descriptor_set);
}

void vk_context::setup_present_queue(VkSurfaceKHR surface)
{
    if (m_present_queue)
    {
        return;
    }

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        m_physical_device,
        m_graphics_queue->get_family_index(),
        surface,
        &present_support);

    if (!present_support)
    {
        throw std::runtime_error("There is no queue that supports presentation.");
    }

    m_present_queue =
        std::make_unique<vk_present_queue>(m_graphics_queue->get_family_index(), this);
}

bool vk_context::initialize_instance(
    std::span<rhi_extension*> extensions,
    vk_extension_registry& registry)
{
    std::uint32_t available_layer_count = 0;
    vk_check(vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr));
    std::vector<VkLayerProperties> available_layers(available_layer_count);
    vk_check(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers.data()));

    std::uint32_t available_extension_count = 0;
    vk_check(vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr));
    std::vector<VkExtensionProperties> available_extensions(available_extension_count);
    vk_check(vkEnumerateInstanceExtensionProperties(
        nullptr,
        &available_extension_count,
        available_extensions.data()));

    for (const auto& layer : available_layers)
    {
        registry.m_available_layers.insert(layer.layerName);
    }

    for (const auto& extension : available_extensions)
    {
        registry.m_available_instance_extensions.insert(extension.extensionName);
    }

    each_extension(
        extensions,
        [&registry](vk_extension* extension)
        {
            if (extension->evaluate_for_instance(registry))
            {
                extension->commit_for_instance(registry);
            }
            else
            {
                extension->m_enabled = false;
            }
        });

    std::vector<const char*> requested_layers(registry.m_requested_layers.size());
    std::ranges::transform(
        registry.m_requested_layers,
        requested_layers.begin(),
        [](const std::string& layer)
        {
            return layer.c_str();
        });

    std::vector<const char*> requested_instance_extensions(
        registry.m_requested_instance_extensions.size());
    std::ranges::transform(
        registry.m_requested_instance_extensions,
        requested_instance_extensions.begin(),
        [](const std::string& extension)
        {
            return extension.c_str();
        });

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "violet app",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "violet engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<std::uint32_t>(requested_layers.size()),
        .ppEnabledLayerNames = requested_layers.empty() ? nullptr : requested_layers.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(requested_instance_extensions.size()),
        .ppEnabledExtensionNames =
            requested_instance_extensions.empty() ? nullptr : requested_instance_extensions.data(),
    };

#ifndef NDEBUG
    std::vector<VkValidationFeatureEnableEXT> validation_features = {
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
    };

    VkValidationFeaturesEXT validation_features_info = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .enabledValidationFeatureCount = static_cast<std::uint32_t>(validation_features.size()),
        .pEnabledValidationFeatures = validation_features.data(),
    };

    VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    debug_info.pfnUserCallback = debug_callback;
    debug_info.pUserData = nullptr;
    debug_info.pNext = &validation_features_info;

    instance_info.pNext = &debug_info;
#endif

    vk_check(vkCreateInstance(&instance_info, nullptr, &m_instance));
    if (m_instance == VK_NULL_HANDLE)
    {
        return false;
    }

    volkLoadInstance(m_instance);

#ifndef NDEBUG
    vk_check(vkCreateDebugUtilsMessengerEXT(m_instance, &debug_info, nullptr, &m_debug_messenger));
#endif

    return true;
}

bool vk_context::initialize_physical_device(
    std::span<rhi_extension*> extensions,
    vk_extension_registry& registry)
{
    std::uint32_t device_count = 0;
    vk_check(vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr));
    std::vector<VkPhysicalDevice> available_devices(device_count);
    vk_check(vkEnumeratePhysicalDevices(m_instance, &device_count, available_devices.data()));

    auto set_registry_device = [&](VkPhysicalDevice device)
    {
        std::uint32_t available_extension_count = 0;
        vk_check(vkEnumerateDeviceExtensionProperties(
            device,
            nullptr,
            &available_extension_count,
            nullptr));
        std::vector<VkExtensionProperties> available_extensions(available_extension_count);
        vk_check(vkEnumerateDeviceExtensionProperties(
            device,
            nullptr,
            &available_extension_count,
            available_extensions.data()));

        registry.m_available_device_extensions.clear();
        for (const auto& extension : available_extensions)
        {
            registry.m_available_device_extensions.insert(extension.extensionName);
        }

        registry.m_available_features.clear();
        registry.m_requested_features.clear();
        registry.m_properties.clear();

        registry.add_feature<VkPhysicalDeviceFeatures2>();
        registry.add_property<VkPhysicalDeviceProperties2>();
        each_extension(
            extensions,
            [&](vk_extension* extension)
            {
                extension->add_device_query(registry);
            });

        vkGetPhysicalDeviceFeatures2(
            device,
            registry.m_available_features.get<VkPhysicalDeviceFeatures2>());
        vkGetPhysicalDeviceProperties2(
            device,
            registry.m_properties.get<VkPhysicalDeviceProperties2>());
    };

    std::uint32_t max_score = 0;
    for (VkPhysicalDevice device : available_devices)
    {
        set_registry_device(device);

        std::uint32_t score = 0;

        each_extension(
            extensions,
            [&](vk_extension* extension)
            {
                if (extension->evaluate_for_device(registry))
                {
                    score += 10;
                }
            });

        const auto& properties = registry.get_property<VkPhysicalDeviceProperties2>();

        if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1000;
        }

        if (max_score < score)
        {
            m_physical_device = device;
            m_physical_device_properties = properties;
            max_score = score;
        }
    }

    set_registry_device(m_physical_device);

    each_extension(
        extensions,
        [&](vk_extension* extension)
        {
            if (extension->evaluate_for_device(registry))
            {
                extension->commit_for_device(registry);
            }
            else
            {
                extension->m_enabled = false;
            }
        });

    const auto* acceleration_structure_properties =
        registry.m_properties.get<VkPhysicalDeviceAccelerationStructurePropertiesKHR>();
    if (acceleration_structure_properties != nullptr)
    {
        m_acceleration_structure_properties = *acceleration_structure_properties;
    }

    const auto* ray_tracing_pipeline_properties =
        registry.m_properties.get<VkPhysicalDeviceRayTracingPipelinePropertiesKHR>();
    if (ray_tracing_pipeline_properties != nullptr)
    {
        m_ray_tracing_pipeline_properties = *ray_tracing_pipeline_properties;
    }

    return m_physical_device != VK_NULL_HANDLE;
}

void vk_context::initialize_device(vk_extension_registry& registry)
{
    std::uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        m_physical_device,
        &queue_family_count,
        queue_families.data());

    std::uint32_t graphics_queue_family_index = -1;
    for (std::uint32_t i = 0; i < queue_families.size(); ++i)
    {
        if (queue_families[i].queueCount != 0 &&
            queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphics_queue_family_index = i;
        }
    }

    std::set<std::uint32_t> queue_indexes = {graphics_queue_family_index};
    std::vector<VkDeviceQueueCreateInfo> queue_infos = {};
    float queue_priority = 1.0;
    for (std::uint32_t index : queue_indexes)
    {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;

        queue_infos.push_back(queue_info);
    }

    std::vector<const char*> requested_device_extensions(
        registry.m_requested_device_extensions.size());
    std::ranges::transform(
        registry.m_requested_device_extensions,
        requested_device_extensions.begin(),
        [](const std::string& extension)
        {
            return extension.c_str();
        });

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = registry.m_requested_features.get<VkPhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = static_cast<std::uint32_t>(queue_infos.size()),
        .pQueueCreateInfos = queue_infos.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(requested_device_extensions.size()),
        .ppEnabledExtensionNames = requested_device_extensions.data(),
    };

    vk_check(vkCreateDevice(m_physical_device, &device_info, nullptr, &m_device));
    volkLoadDevice(m_device);

    m_graphics_queue = std::make_unique<vk_graphics_queue>(graphics_queue_family_index, this);
}

void vk_context::initialize_vma(vk_extension_registry& registry)
{
    VmaVulkanFunctions vulkan_functions = {};
    vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    vulkan_functions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vulkan_functions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vulkan_functions.vkAllocateMemory = vkAllocateMemory;
    vulkan_functions.vkFreeMemory = vkFreeMemory;
    vulkan_functions.vkMapMemory = vkMapMemory;
    vulkan_functions.vkUnmapMemory = vkUnmapMemory;
    vulkan_functions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vulkan_functions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vulkan_functions.vkBindBufferMemory = vkBindBufferMemory;
    vulkan_functions.vkBindImageMemory = vkBindImageMemory;
    vulkan_functions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vulkan_functions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vulkan_functions.vkCreateBuffer = vkCreateBuffer;
    vulkan_functions.vkDestroyBuffer = vkDestroyBuffer;
    vulkan_functions.vkCreateImage = vkCreateImage;
    vulkan_functions.vkDestroyImage = vkDestroyImage;
    vulkan_functions.vkCmdCopyBuffer = vkCmdCopyBuffer;
    vulkan_functions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
    vulkan_functions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
    vulkan_functions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
    vulkan_functions.vkBindImageMemory2KHR = vkBindImageMemory2;
    vulkan_functions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
    vulkan_functions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
    vulkan_functions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

    VmaAllocatorCreateFlags flags = 0;

    const auto* vk12_features =
        registry.m_requested_features.get<VkPhysicalDeviceVulkan12Features>();
    if (vk12_features != nullptr)
    {
        if (vk12_features->bufferDeviceAddress)
        {
            flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }
    }

    VmaAllocatorCreateInfo allocator_info = {
        .flags = flags,
        .physicalDevice = m_physical_device,
        .device = m_device,
        .pVulkanFunctions = &vulkan_functions,
        .instance = m_instance,
        .vulkanApiVersion = VK_API_VERSION_1_3,
    };

    vk_check(vmaCreateAllocator(&allocator_info, &m_vma_allocator));
}

void vk_context::initialize_descriptor_pool(bool bindless)
{
    std::vector<VkDescriptorPoolSize> pool_size = {
        {.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = 1024},
        {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1024},
        {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1024},
        {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1024},
        {.type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 1024},
        {.type = VK_DESCRIPTOR_TYPE_MUTABLE_EXT, .descriptorCount = 65536},
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 512,
        .poolSizeCount = static_cast<std::uint32_t>(pool_size.size()),
        .pPoolSizes = pool_size.data(),
    };

    if (bindless)
    {
        pool_info.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    }

    vk_check(vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_descriptor_pool));
}
} // namespace violet::vk