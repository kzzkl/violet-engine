#pragma once

#include "vk_common.hpp"

namespace ash::graphics::vk
{
class vk_swap_chain;
class vk_render_pass;
class vk_command_queue;
class vk_descriptor_pool;
class vk_sampler;

class vk_frame_counter
{
public:
    static void initialize(std::size_t frame_counter, std::size_t frame_resousrce_count) noexcept;

    static void tick() noexcept;

    inline static std::size_t frame_counter() noexcept { return instance().m_frame_counter; }
    inline static std::size_t frame_resource_count() noexcept
    {
        return instance().m_frame_resousrce_count;
    }
    inline static std::size_t frame_resource_index() noexcept
    {
        auto& context = instance();
        return context.m_frame_counter % context.m_frame_resousrce_count;
    }

private:
    vk_frame_counter() noexcept;
    static vk_frame_counter& instance() noexcept;

    std::size_t m_frame_counter;
    std::size_t m_frame_resousrce_count;
};

class vk_context
{
public:
    struct command_queue_index
    {
        std::uint32_t graphics;
        std::uint32_t present;
    };

public:
    static bool initialize(const renderer_desc& desc) { return instance().on_initialize(desc); }

    static void deinitialize() { instance().on_deinitialize(); }

    static VkInstance vk_instance() noexcept { return instance().m_instance; }
    static VkPhysicalDevice physical_device() noexcept { return instance().m_physical_device; }
    static VkDevice device() noexcept { return instance().m_device; }

    static const command_queue_index& queue_index() noexcept { return instance().m_queue_index; }

    static vk_swap_chain& swap_chain() { return *instance().m_swap_chain; }

    static vk_descriptor_pool& descriptor_pool() { return *instance().m_descriptor_pool; }

    static vk_command_queue& graphics_queue() { return *instance().m_graphics_queue; }
    static vk_command_queue& present_queue() { return *instance().m_present_queue; }

    static VkSemaphore image_available_semaphore() noexcept { return instance().m_image_available; }
    static VkSemaphore render_finished_semaphore() noexcept { return instance().m_render_finished; }

    static VkFence fence() noexcept { return instance().m_fence; }

    static vk_sampler& sampler() { return *instance().m_sampler; }

    static std::size_t begin_frame() { return instance().on_begin_frame(); }
    static void end_frame() { instance().on_end_frame(); }

private:
    vk_context();
    static vk_context& instance();

    bool on_initialize(const renderer_desc& desc);
    void on_deinitialize();
    std::size_t on_begin_frame();
    void on_end_frame();

    void create_instance();
    void create_surface(void* window_handle);
    void create_device();
    void create_swap_chain(std::uint32_t width, std::uint32_t height);
    void create_command_queue();
    void create_semaphore();
    void create_descriptor_pool();
    void create_sampler();

    bool check_validation_layer();
    bool check_device_extension_support(
        VkPhysicalDevice device,
        const std::vector<const char*>& extensions);
    bool check_device_swap_chain_support(VkPhysicalDevice device);

    VkInstance m_instance;

    VkPhysicalDevice m_physical_device;
    VkDevice m_device;
    command_queue_index m_queue_index;

    std::unique_ptr<vk_command_queue> m_graphics_queue;
    std::unique_ptr<vk_command_queue> m_present_queue;

    VkSurfaceKHR m_surface;
    std::unique_ptr<vk_swap_chain> m_swap_chain;

    std::unique_ptr<vk_descriptor_pool> m_descriptor_pool;

    VkSemaphore m_image_available;
    VkSemaphore m_render_finished;
    VkFence m_fence;

    std::unique_ptr<vk_sampler> m_sampler;

    std::uint32_t m_image_index;

#ifndef NODEBUG
    VkDebugUtilsMessengerEXT m_debug_callback;
#endif
};
} // namespace ash::graphics::vk