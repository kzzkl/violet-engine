#include "extensions/core_extension.hpp"
#include <algorithm>

#ifdef VIOLET_ENABLE_VULKAN
#include "rhi/vulkan/rhi_vulkan.hpp"
#endif

namespace violet
{
#ifdef VIOLET_ENABLE_VULKAN
class core_extension_vk : public vk::vk_extension
{
public:
    bool evaluate_for_instance(vk::vk_extension_registry& registry) override
    {
#ifndef NDEBUG
        if (registry.is_layer_available("VK_LAYER_KHRONOS_validation"))
        {
            registry.request_layer("VK_LAYER_KHRONOS_validation");
        }

        if (registry.is_instance_extension_available(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            registry.request_instance_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
#endif

        std::vector<const char*> requested_instance_extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
        };

        for (const char* extension : requested_instance_extensions)
        {
            if (!registry.is_instance_extension_available(extension))
            {
                return false;
            }
        }

        for (const char* extension : requested_instance_extensions)
        {
            registry.request_instance_extension(extension);
        }

        return true;
    }

    bool evaluate_for_device(vk::vk_extension_registry& registry) override
    {
        bool result = true;

        std::vector<const char*> requested_device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
            VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME, // For bindless
        // VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME,
#ifndef NDEBUG
            VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME, // For shader debug.
#endif
        };

        result = std::ranges::all_of(
            requested_device_extensions,
            [&](const char* extension)
            {
                return registry.is_device_extension_available(extension);
            });

        if (result)
        {
            const auto& available = registry.get_available_feature<VkPhysicalDeviceFeatures2>();
            result = available.features.geometryShader && available.features.fillModeNonSolid &&
                     available.features.samplerAnisotropy &&
                     available.features.fragmentStoresAndAtomics;
        }

        if (result)
        {
            const auto& available =
                registry.get_available_feature<VkPhysicalDeviceVulkan12Features>();

            result = available.samplerFilterMinmax && // For HZB
                     available.hostQueryReset && available.timelineSemaphore &&
                     available.drawIndirectCount &&                             // For draw indirect
                     available.runtimeDescriptorArray &&                        // For bindless
                     available.shaderUniformBufferArrayNonUniformIndexing &&    // For bindless
                     available.shaderSampledImageArrayNonUniformIndexing &&     // For bindless
                     available.shaderStorageBufferArrayNonUniformIndexing &&    // For bindless
                     available.descriptorBindingUniformBufferUpdateAfterBind && // For bindless
                     available.descriptorBindingSampledImageUpdateAfterBind &&  // For bindless
                     available.descriptorBindingStorageBufferUpdateAfterBind && // For bindless
                     available.descriptorBindingPartiallyBound;                 // For bindless
        }

        if (result)
        {
            const auto& available =
                registry.get_available_feature<VkPhysicalDeviceVulkan13Features>();

            result = available.shaderDemoteToHelperInvocation && // For hlsl clip, alpha test.
                     available.synchronization2;
        }

        if (result)
        {
            const auto& available =
                registry.get_available_feature<VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT>();

            result = available.mutableDescriptorType; // For bindless
        }

#ifndef NDEBUG
        if (result)
        {
            const auto& available = registry.get_available_feature<
                VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR>();
            result = available.shaderRelaxedExtendedInstruction;
        }
#endif

        if (result)
        {
            for (const char* extension : requested_device_extensions)
            {
                registry.request_device_extension(extension);
            }

            {
                auto& requested = registry.get_requested_feature<VkPhysicalDeviceFeatures2>();
                requested.features.geometryShader = VK_TRUE;
                requested.features.fillModeNonSolid = VK_TRUE;
                requested.features.samplerAnisotropy = VK_TRUE;
                requested.features.fragmentStoresAndAtomics = VK_TRUE;
            }

            {
                auto& requested =
                    registry.get_requested_feature<VkPhysicalDeviceVulkan12Features>();
                requested.samplerFilterMinmax = VK_TRUE;
                requested.hostQueryReset = VK_TRUE;
                requested.timelineSemaphore = VK_TRUE;
                requested.drawIndirectCount = VK_TRUE;
                requested.runtimeDescriptorArray = VK_TRUE;
                requested.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
                requested.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
                requested.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
                requested.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
                requested.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
                requested.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
                requested.descriptorBindingPartiallyBound = VK_TRUE;
            }

            {
                auto& requested =
                    registry.get_requested_feature<VkPhysicalDeviceVulkan13Features>();
                requested.shaderDemoteToHelperInvocation = VK_TRUE;
                requested.synchronization2 = VK_TRUE;
            }

            {
                auto& requested =
                    registry
                        .get_requested_feature<VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT>();
                requested.mutableDescriptorType = VK_TRUE;
            }

#ifndef NDEBUG
            {
                auto& requested = registry.get_requested_feature<
                    VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR>();
                requested.shaderRelaxedExtendedInstruction = VK_TRUE;
            }
#endif
        }

        return result;
    }

    void add_feature_query(vk::vk_extension_registry& registry) const override
    {
        registry.add_feature<VkPhysicalDeviceVulkan12Features>();
        registry.add_feature<VkPhysicalDeviceVulkan13Features>();
        registry.add_feature<VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT>();

#ifndef NDEBUG
        registry.add_feature<VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR>();
#endif
    }
};
#endif

core_extension::core_extension()
    : render_device_extension("Core")
{
}

std::unique_ptr<rhi_extension> core_extension::create_rhi(rhi_backend backend)
{
    switch (backend)
    {
#ifdef VIOLET_ENABLE_VULKAN
    case RHI_BACKEND_VULKAN:
        return std::make_unique<core_extension_vk>();
#endif
    default:
        return nullptr;
    }
}
} // namespace violet