#pragma once

#include "vk_common.hpp"

namespace ash::graphics::vk
{
class vk_swap_chain;
class vk_pipeline;
class vk_command_queue;
class vk_renderer;

class vk_context
{
public:
    struct command_queue_index
    {
        std::uint32_t graphics;
        std::uint32_t present;
    };

public:
    static bool initialize(const context_config& config)
    {
        return instance().do_initialize(config);
    }

    static void deinitialize() { instance().do_deinitialize(); }

    static VkInstance vk_instance() noexcept { return instance().m_instance; }
    static VkPhysicalDevice physical_device() noexcept { return instance().m_physical_device; }
    static VkDevice device() noexcept { return instance().m_device; }

    static const command_queue_index& queue_index() noexcept { return instance().m_queue_index; }

    static vk_swap_chain& swap_chain() { return *instance().m_swap_chain; }
    static vk_renderer& renderer() { return *instance().m_renderer; }

    static vk_command_queue& graphics_queue() { return *instance().m_graphics_queue; }
    static vk_command_queue& present_queue() { return *instance().m_present_queue; }

    static VkSemaphore image_available_semaphore() noexcept { return instance().m_image_available; }
    static VkSemaphore render_finished_semaphore() noexcept { return instance().m_render_finished; }

    static VkFence fence() noexcept { return instance().m_fence; }

    static void begin_frame() { instance().do_begin_frame(); }
    static void end_frame() { instance().do_end_frame(); }

    static std::uint32_t image_index() noexcept { return instance().m_image_index; }

private:
    vk_context();
    static vk_context& instance();

    bool do_initialize(const context_config& config);
    void do_deinitialize();
    void do_begin_frame();
    void do_end_frame();

    void create_instance();
    void create_surface(void* window_handle);
    void create_device();
    void create_swap_chain(std::uint32_t width, std::uint32_t height);
    void create_command_queue();
    void create_semaphore();

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

    std::unique_ptr<vk_renderer> m_renderer;
    std::unique_ptr<vk_pipeline> m_pipeline;

    VkSemaphore m_image_available;
    VkSemaphore m_render_finished;
    VkFence m_fence;

    std::uint32_t m_image_index;

#ifndef NODEBUG
    VkDebugUtilsMessengerEXT m_debug_callback;
#endif
};
} // namespace ash::graphics::vk