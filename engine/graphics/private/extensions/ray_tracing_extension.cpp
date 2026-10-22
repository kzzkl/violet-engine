#include "graphics/extensions/ray_tracing_extension.hpp"

#ifdef VIOLET_ENABLE_VULKAN
#include "rhi/vulkan/rhi_vulkan.hpp"
#endif

namespace violet
{
#ifdef VIOLET_ENABLE_VULKAN
class ray_tracing_extension_vk : public vk::vk_extension
{
public:
    bool evaluate_for_device(const vk::vk_extension_registry& registry) const override
    {
        bool result = registry.check_device_extensions(get_requested_device_extensions());

        if (result)
        {
            const auto& available =
                registry.get_available_feature<VkPhysicalDeviceVulkan12Features>();
            result = available.bufferDeviceAddress;
        }

        if (result)
        {
            const auto& available =
                registry.get_available_feature<VkPhysicalDeviceAccelerationStructureFeaturesKHR>();
            result = available.accelerationStructure;
        }

        if (result)
        {
            const auto& available =
                registry.get_available_feature<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>();
            result = available.rayTracingPipeline;
        }

        return result;
    }

    void commit_for_device(vk::vk_extension_registry& registry) override
    {
        for (const char* extension : get_requested_device_extensions())
        {
            registry.request_device_extension(extension);
        }

        {
            auto& requested = registry.get_requested_feature<VkPhysicalDeviceVulkan12Features>();
            requested.bufferDeviceAddress = VK_TRUE;
        }

        {
            auto& requested =
                registry.get_requested_feature<VkPhysicalDeviceAccelerationStructureFeaturesKHR>();
            requested.accelerationStructure = VK_TRUE;
        }

        {
            auto& requested =
                registry.get_requested_feature<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>();
            requested.rayTracingPipeline = VK_TRUE;
        }

        m_rtProperties = registry.get_property<VkPhysicalDeviceRayTracingPipelinePropertiesKHR>();
        m_asProperties =
            registry.get_property<VkPhysicalDeviceAccelerationStructurePropertiesKHR>();
    }

    void add_device_query(vk::vk_extension_registry& registry) const override
    {
        registry.add_feature<VkPhysicalDeviceVulkan12Features>();
        registry.add_feature<VkPhysicalDeviceAccelerationStructureFeaturesKHR>();
        registry.add_feature<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>();

        registry.add_property<VkPhysicalDeviceAccelerationStructurePropertiesKHR>();
        registry.add_property<VkPhysicalDeviceRayTracingPipelinePropertiesKHR>();
    }

private:
    std::vector<const char*> get_requested_device_extensions() const
    {
        return {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,   // Build acceleration structures
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,     // Use vkCmdTraceRaysKHR
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, // Required by ray tracing pipeline
        };
    }

    VkPhysicalDeviceAccelerationStructurePropertiesKHR m_asProperties;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties;
};
#endif

ray_tracing_extension::ray_tracing_extension()
    : render_device_extension("RayTracing")
{
}

std::unique_ptr<rhi_extension> ray_tracing_extension::create_rhi(rhi_backend backend)
{
    switch (backend)
    {
#ifdef VIOLET_ENABLE_VULKAN
    case RHI_BACKEND_VULKAN:
        return std::make_unique<ray_tracing_extension_vk>();
#endif
    default:
        return nullptr;
    }
}
} // namespace violet