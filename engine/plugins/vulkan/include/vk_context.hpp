#pragma once

#include "vk_common.hpp"
#include <memory>
#include <vector>

namespace violet::vk
{
class vk_graphics_queue;
class vk_present_queue;

class vk_context
{
public:
    vk_context() noexcept;
    vk_context(const vk_context&) = delete;
    ~vk_context();

    bool initialize(const rhi_desc& desc);

    void next_frame() noexcept;

    VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout);
    void free_descriptor_set(VkDescriptorSet descriptor_set);

    vk_graphics_queue* get_graphics_queue() const noexcept { return m_graphics_queue.get(); }
    vk_present_queue* get_present_queue() const noexcept { return m_present_queue.get(); }

    VkDevice get_device() const noexcept { return m_device; }

    VkPhysicalDevice get_physical_device() const noexcept { return m_physical_device; }
    VkPhysicalDeviceProperties get_physical_device_properties() const noexcept
    {
        return m_physical_device_properties;
    }

    VkSurfaceKHR get_surface() const noexcept { return m_surface; }

    std::size_t get_frame_count() const noexcept { return m_frame_count; }
    std::size_t get_frame_resource_count() const noexcept { return m_frame_resource_count; }
    std::size_t get_frame_resource_index() const noexcept { return m_frame_resource_index; }

    vk_context& operator=(const vk_context&) = delete;

private:
    bool initialize_instance(
        const std::vector<const char*>& desired_layers,
        const std::vector<const char*>& desired_extensions);
    bool initialize_physical_device(const std::vector<const char*>& desired_extensions);
    void initialize_logic_device(const std::vector<const char*>& enabled_extensions);
    void initialize_descriptor_pool();

    VkInstance m_instance;
    VkPhysicalDevice m_physical_device;
    VkPhysicalDeviceProperties m_physical_device_properties;
    VkDevice m_device;

    VkSurfaceKHR m_surface;

    std::unique_ptr<vk_graphics_queue> m_graphics_queue;
    std::unique_ptr<vk_present_queue> m_present_queue;

    VkDescriptorPool m_descriptor_pool;

    std::size_t m_frame_count;
    std::size_t m_frame_resource_count;
    std::size_t m_frame_resource_index;

#ifndef NDEUBG
    VkDebugUtilsMessengerEXT m_debug_messenger;
#endif
};
} // namespace violet::vk