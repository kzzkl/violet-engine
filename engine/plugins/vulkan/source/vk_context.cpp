#include "vk_context.hpp"
#include "vk_bindless.hpp"
#include "vk_command.hpp"
#include "vk_framebuffer.hpp"
#include "vk_layout.hpp"
#include "vk_parameter.hpp"
#include <iostream>
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
    std::cout << "validation layer: " << data->pMessage << std::endl;
    return VK_FALSE;
}

template <typename T>
void enable_feature(VkPhysicalDeviceFeatures2& device_features, T& feature)
{
    feature.pNext = device_features.pNext;
    device_features.pNext = &feature;
};

bool check_extension_support(
    std::span<const char*> desired_extensions,
    std::span<VkExtensionProperties> available_extensions)
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
        {
            return false;
        }
    }

    return true;
}

bool check_feature_support(rhi_features desired_features, VkPhysicalDevice device)
{
    VkPhysicalDeviceFeatures2 device_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    };

    VkPhysicalDeviceVulkan12Features vulkan12_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    };
    enable_feature(device_features, vulkan12_features);

    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutable_descriptor_type_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT,
    };
    if (desired_features & RHI_FEATURE_BINDLESS)
    {
        enable_feature(device_features, mutable_descriptor_type_features);
    }

    vkGetPhysicalDeviceFeatures2(device, &device_features);

    if (!device_features.features.samplerAnisotropy || !vulkan12_features.timelineSemaphore)
    {
        return false;
    }

    if (desired_features & RHI_FEATURE_BINDLESS)
    {
        if (!vulkan12_features.runtimeDescriptorArray ||
            !vulkan12_features.shaderSampledImageArrayNonUniformIndexing ||
            !vulkan12_features.descriptorBindingSampledImageUpdateAfterBind ||
            !vulkan12_features.shaderUniformBufferArrayNonUniformIndexing ||
            !vulkan12_features.descriptorBindingUniformBufferUpdateAfterBind ||
            !vulkan12_features.shaderStorageBufferArrayNonUniformIndexing ||
            !vulkan12_features.descriptorBindingStorageBufferUpdateAfterBind)
        {
            return false;
        }

        if (!mutable_descriptor_type_features.mutableDescriptorType)
        {
            return false;
        }
    }

    return true;
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

    std::vector<const char*> instance_desired_layers;
    std::vector<const char*> instance_desired_extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
    };

#ifdef _WIN32
    instance_desired_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

#ifndef NDEBUG
    instance_desired_layers.push_back("VK_LAYER_KHRONOS_validation");
    instance_desired_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    if (!initialize_instance(instance_desired_layers, instance_desired_extensions))
    {
        return false;
    }

    std::vector<const char*> device_desired_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        // VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME,
    };

    if (desc.features & RHI_FEATURE_BINDLESS)
    {
        device_desired_extensions.push_back(VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME);
    }

    if (!initialize_physical_device(desc.features, device_desired_extensions))
    {
        return false;
    }

    initialize_logic_device(desc.features, device_desired_extensions);
    initialize_vma();
    initialize_descriptor_pool(desc.features & RHI_FEATURE_BINDLESS);

    m_deletion_queue = std::make_unique<vk_deletion_queue>(this);

    m_layout_manager = std::make_unique<vk_layout_manager>(this);
    m_parameter_manager = std::make_unique<vk_parameter_manager>(this);
    m_bindless_manager = std::make_unique<vk_bindless_manager>(this);
    m_framebuffer_manager = std::make_unique<vk_framebuffer_manager>(this);

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
    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = m_descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;

    std::lock_guard lock(m_mutex);

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
    std::span<const char*> desired_layers,
    std::span<const char*> desired_extensions)
{
    std::uint32_t available_layer_count = 0;
    vk_check(vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr));
    std::vector<VkLayerProperties> available_layers(available_layer_count);
    vk_check(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers.data()));

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
        {
            return false;
        }
    }

    std::uint32_t available_extension_count = 0;
    vk_check(vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr));
    std::vector<VkExtensionProperties> available_extensions(available_extension_count);
    vk_check(vkEnumerateInstanceExtensionProperties(
        nullptr,
        &available_extension_count,
        available_extensions.data()));
    if (!check_extension_support(desired_extensions, available_extensions))
    {
        return false;
    }

    VkApplicationInfo app_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "violet app",
        VK_MAKE_VERSION(1, 0, 0),
        "violet engine",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_3};

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
    rhi_features desired_features,
    std::span<const char*> desired_extensions)
{
    std::uint32_t devices_count = 0;
    vk_check(vkEnumeratePhysicalDevices(m_instance, &devices_count, nullptr));
    std::vector<VkPhysicalDevice> available_devices(devices_count);
    vk_check(vkEnumeratePhysicalDevices(m_instance, &devices_count, available_devices.data()));

    std::uint32_t physical_device_score = 0;
    for (VkPhysicalDevice device : available_devices)
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

        if (!check_extension_support(desired_extensions, available_extensions) ||
            !check_feature_support(desired_features, device))
        {
            continue;
        }

        std::uint32_t score = 0;

        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(device, &properties);

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1000;
        }

        score += properties.limits.maxImageDimension2D;

        if (physical_device_score < score)
        {
            m_physical_device = device;
            m_physical_device_properties = properties;
            physical_device_score = score;
        }
    }

    return m_physical_device != VK_NULL_HANDLE;
}

void vk_context::initialize_logic_device(
    rhi_features desired_features,
    std::span<const char*> enabled_extensions)
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

    VkPhysicalDeviceFeatures2 device_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .features =
            {
                .geometryShader = VK_TRUE,
                .fillModeNonSolid = VK_TRUE,
                .samplerAnisotropy = VK_TRUE,
            },
    };

    VkPhysicalDeviceVulkan12Features vulkan12_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .samplerFilterMinmax = VK_TRUE,
        .timelineSemaphore = VK_TRUE,
    };

    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutable_descriptor_type_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT,
    };

    if (desired_features & RHI_FEATURE_INDIRECT_DRAW || desired_features & RHI_FEATURE_BINDLESS)
    {
        if (desired_features & RHI_FEATURE_INDIRECT_DRAW)
        {
            vulkan12_features.drawIndirectCount = VK_TRUE;
        }

        if (desired_features & RHI_FEATURE_BINDLESS)
        {
            vulkan12_features.runtimeDescriptorArray = VK_TRUE;
            vulkan12_features.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
            vulkan12_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
            vulkan12_features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
            vulkan12_features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
            vulkan12_features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
            vulkan12_features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
            vulkan12_features.descriptorBindingPartiallyBound = VK_TRUE;

            mutable_descriptor_type_features.mutableDescriptorType = VK_TRUE;
        }

        enable_feature(device_features, vulkan12_features);
        enable_feature(device_features, mutable_descriptor_type_features);
    }

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &device_features,
        .queueCreateInfoCount = static_cast<std::uint32_t>(queue_infos.size()),
        .pQueueCreateInfos = queue_infos.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(enabled_extensions.size()),
        .ppEnabledExtensionNames = enabled_extensions.data(),
    };

    vk_check(vkCreateDevice(m_physical_device, &device_info, nullptr, &m_device));
    volkLoadDevice(m_device);

    m_graphics_queue = std::make_unique<vk_graphics_queue>(graphics_queue_family_index, this);
}

void vk_context::initialize_vma()
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

    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_info.physicalDevice = m_physical_device;
    allocator_info.device = m_device;
    allocator_info.instance = m_instance;
    allocator_info.pVulkanFunctions = &vulkan_functions;

    vk_check(vmaCreateAllocator(&allocator_info, &m_vma_allocator));
}

void vk_context::initialize_descriptor_pool(bool bindless)
{
    std::vector<VkDescriptorPoolSize> pool_size = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024}};

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