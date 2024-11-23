#pragma once

#include "vk_common.hpp"
#include "vk_mem_alloc.h"
#include <memory>
#include <span>

namespace violet::vk
{
class vk_graphics_queue;
class vk_present_queue;
class vk_layout_manager;

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
    bool initialize_physical_device(std::span<const char*> desired_extensions);
    void initialize_logic_device(std::span<const char*> enabled_extensions);
    void initialize_vma();
    void initialize_descriptor_pool();

    VkInstance m_instance{VK_NULL_HANDLE};
    VkPhysicalDevice m_physical_device{VK_NULL_HANDLE};
    VkPhysicalDeviceProperties m_physical_device_properties;
    VkDevice m_device{VK_NULL_HANDLE};

    std::unique_ptr<vk_graphics_queue> m_graphics_queue;
    std::unique_ptr<vk_present_queue> m_present_queue;
    std::unique_ptr<vk_layout_manager> m_layout_manager;

    VkDescriptorPool m_descriptor_pool{VK_NULL_HANDLE};

    VmaAllocator m_vma_allocator;

    std::size_t m_frame_count{0};
    std::size_t m_frame_resource_count{0};
    std::size_t m_frame_resource_index{0};

#ifndef NDEUBG
    VkDebugUtilsMessengerEXT m_debug_messenger{VK_NULL_HANDLE};
#endif
};
} // namespace violet::vk