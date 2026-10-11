#pragma once

#include <volk.h>

namespace violet::vk
{
template <typename T>
struct vk_structure_traits;

template <>
struct vk_structure_traits<VkPhysicalDeviceFeatures2>
{
    static constexpr VkStructureType type = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
};

template <>
struct vk_structure_traits<VkPhysicalDeviceVulkan12Features>
{
    static constexpr VkStructureType type = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
};

template <>
struct vk_structure_traits<VkPhysicalDeviceVulkan13Features>
{
    static constexpr VkStructureType type = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
};

template <>
struct vk_structure_traits<VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT>
{
    static constexpr VkStructureType type =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
};

template <>
struct vk_structure_traits<VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR>
{
    static constexpr VkStructureType type =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_RELAXED_EXTENDED_INSTRUCTION_FEATURES_KHR;
};
} // namespace violet::vk