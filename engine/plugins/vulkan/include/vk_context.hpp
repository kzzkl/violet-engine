#pragma once

#include "vk_common.hpp"
#include "vk_deletion_queue.hpp"
#include <memory>
#include <mutex>
#include <span>

namespace violet::vk
{
class vk_graphics_queue;
class vk_present_queue;
class vk_layout_manager;
class vk_parameter_manager;
class vk_parameter;
class vk_bindless_manager;
class vk_framebuffer_manager;
class vk_deletion_queue;

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

    void setup_present_queue(VkSurfaceKHR surface);

    vk_graphics_queue* get_graphics_queue() const noexcept
    {
        return m_graphics_queue.get();
    }

    vk_present_queue* get_present_queue() const noexcept
    {
        return m_present_queue.get();
    }

    vk_layout_manager* get_layout_manager() const noexcept
    {
        return m_layout_manager.get();
    }

    vk_parameter_manager* get_parameter_manager() const noexcept
    {
        return m_parameter_manager.get();
    }

    vk_bindless_manager* get_bindless_manager() const noexcept
    {
        return m_bindless_manager.get();
    }

    vk_framebuffer_manager* get_framebuffer_manager() const noexcept
    {
        return m_framebuffer_manager.get();
    }

    vk_deletion_queue* get_deletion_queue() noexcept
    {
        return m_deletion_queue.get();
    }

    VkInstance get_instance() const noexcept
    {
        return m_instance;
    }

    VkDevice get_device() const noexcept
    {
        return m_device;
    }

    VkPhysicalDevice get_physical_device() const noexcept
    {
        return m_physical_device;
    }

    VkPhysicalDeviceProperties get_physical_device_properties() const noexcept
    {
        return m_physical_device_properties;
    }

    std::size_t get_frame_count() const noexcept
    {
        return m_frame_count;
    }

    std::size_t get_frame_resource_count() const noexcept
    {
        return m_frame_resource_count;
    }

    std::size_t get_frame_resource_index() const noexcept
    {
        return m_frame_resource_index;
    }

    VmaAllocator get_vma_allocator() const noexcept
    {
        return m_vma_allocator;
    }

    vk_context& operator=(const vk_context&) = delete;

private:
    bool initialize_instance(
        std::span<const char*> desired_layers,
        std::span<const char*> desired_extensions);
    bool initialize_physical_device(
        rhi_features desired_features,
        std::span<const char*> desired_extensions);
    void initialize_logic_device(
        rhi_features desired_features,
        std::span<const char*> enabled_extensions);
    void initialize_vma();
    void initialize_descriptor_pool(bool bindless);

    VkInstance m_instance{VK_NULL_HANDLE};
    VkPhysicalDevice m_physical_device{VK_NULL_HANDLE};
    VkPhysicalDeviceProperties m_physical_device_properties;
    VkDevice m_device{VK_NULL_HANDLE};

    std::unique_ptr<vk_graphics_queue> m_graphics_queue;
    std::unique_ptr<vk_present_queue> m_present_queue;

    std::unique_ptr<vk_layout_manager> m_layout_manager;
    std::unique_ptr<vk_parameter_manager> m_parameter_manager;
    std::unique_ptr<vk_bindless_manager> m_bindless_manager;
    std::unique_ptr<vk_framebuffer_manager> m_framebuffer_manager;

    std::unique_ptr<vk_deletion_queue> m_deletion_queue;

    VkDescriptorPool m_descriptor_pool{VK_NULL_HANDLE};

    VmaAllocator m_vma_allocator;

    std::size_t m_frame_count{0};
    std::size_t m_frame_resource_count{0};
    std::size_t m_frame_resource_index{0};

    std::mutex m_mutex;

#ifndef NDEUBG
    VkDebugUtilsMessengerEXT m_debug_messenger{VK_NULL_HANDLE};
#endif
};
} // namespace violet::vk