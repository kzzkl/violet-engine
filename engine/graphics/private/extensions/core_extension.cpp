#include "extensions/core_extension.hpp"

#ifdef VIOLET_ENABLE_VULKAN
#include "rhi/vulkan/rhi_vulkan.hpp"
#endif

namespace violet
{
#ifdef VIOLET_ENABLE_VULKAN
class core_extension_vk : public vk::vk_extension
{
public:
    bool evaluate_for_instance(const vk::vk_extension_registry& registry) const override
    {
        return registry.check_instance_extensions(get_requested_instance_extensions());
    }

    void commit_for_instance(vk::vk_extension_registry& registry) override
    {
        registry.request_instance_extensions(get_requested_instance_extensions());

#ifndef NDEBUG
        if (registry.check_layer("VK_LAYER_KHRONOS_validation"))
        {
            registry.request_layer("VK_LAYER_KHRONOS_validation");
        }

        if (registry.check_instance_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            registry.request_instance_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
#endif
    }

    bool evaluate_for_device(const vk::vk_extension_registry& registry) const override
    {
        bool result = registry.check_device_extensions(get_requested_device_extensions());

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

        return result;
    }

    void commit_for_device(vk::vk_extension_registry& registry) override
    {
        for (const char* extension : get_requested_device_extensions())
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
            auto& requested = registry.get_requested_feature<VkPhysicalDeviceVulkan12Features>();
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
            auto& requested = registry.get_requested_feature<VkPhysicalDeviceVulkan13Features>();
            requested.shaderDemoteToHelperInvocation = VK_TRUE;
            requested.synchronization2 = VK_TRUE;
        }

        {
            auto& requested =
                registry.get_requested_feature<VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT>();
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

    void add_device_query(vk::vk_extension_registry& registry) const override
    {
        registry.add_feature<VkPhysicalDeviceVulkan12Features>();
        registry.add_feature<VkPhysicalDeviceVulkan13Features>();
        registry.add_feature<VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT>();

#ifndef NDEBUG
        registry.add_feature<VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR>();
#endif
    }

private:
    std::vector<const char*> get_requested_instance_extensions() const
    {
        return {
            VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
        };
    }

    std::vector<const char*> get_requested_device_extensions() const
    {
        return {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
            VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME, // For bindless
        // VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME,
#ifndef NDEBUG
            VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME, // For shader debug.
#endif
        };
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