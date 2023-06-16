#pragma once

#include "vk_common.hpp"
#include "vk_resource.hpp"
#include <memory>
#include <vector>

namespace violet::vk
{
class vk_command_queue;
class vk_rhi : public rhi_context
{
public:
    vk_rhi() noexcept;
    vk_rhi(const vk_rhi&) = delete;
    virtual ~vk_rhi();

    virtual bool initialize(const rhi_desc& desc) override;

    virtual rhi_render_command* allocate_command() override;
    virtual void execute(rhi_render_command* command, rhi_fence* fence) override;

    virtual void begin_frame() override;
    virtual void end_frame() override;
    virtual void present() override;

    virtual void resize(std::uint32_t width, std::uint32_t height) override {}

    virtual rhi_resource* get_back_buffer() override;

    vk_rhi& operator=(const vk_rhi&) = delete;

public:
    virtual rhi_render_pass* make_render_pass(const rhi_render_pass_desc& desc) override;
    virtual void destroy_render_pass(rhi_render_pass* render_pass) override;

    virtual rhi_render_pipeline* make_render_pipeline(
        const rhi_render_pipeline_desc& desc) override;
    virtual void destroy_render_pipeline(rhi_render_pipeline* render_pipeline) override;

    virtual rhi_pipeline_parameter* make_pipeline_parameter(
        const rhi_pipeline_parameter_desc& desc) override;
    virtual void destroy_pipeline_parameter(rhi_pipeline_parameter* pipeline_parameter) override;

    virtual rhi_framebuffer* make_framebuffer(const rhi_framebuffer_desc& desc) override;
    virtual void destroy_framebuffer(rhi_framebuffer* framebuffer) override;

    virtual rhi_resource* make_vertex_buffer(const rhi_vertex_buffer_desc& desc) override;
    virtual rhi_resource* make_index_buffer(const rhi_index_buffer_desc& desc) override;

    virtual rhi_resource* make_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height,
        rhi_resource_format format) override;
    virtual rhi_resource* make_texture(const char* file) override;
    virtual rhi_resource* make_texture_cube(
        const char* left,
        const char* right,
        const char* top,
        const char* bottom,
        const char* front,
        const char* back) override;

    virtual rhi_resource* make_shadow_map(const rhi_shadow_map_desc& desc) override;

    virtual rhi_resource* make_render_target(const rhi_render_target_desc& desc) override;
    virtual rhi_resource* make_depth_stencil_buffer(
        const rhi_depth_stencil_buffer_desc& desc) override;

    virtual rhi_fence* make_fence() override;
    virtual void destroy_fence(rhi_fence* fence) override;

    virtual rhi_semaphore* make_semaphore() override;
    virtual void destroy_semaphore(rhi_semaphore* semaphore) override;

public:
    VkDevice get_device() const noexcept { return m_device; }

    std::size_t get_frame_resource_count() const noexcept { return m_frame_resource_count; }
    std::size_t get_frame_resource_index() const noexcept { return m_frame_resource_index; }

private:
    struct queue_family_indices
    {
        std::uint32_t graphics;
        std::uint32_t present;
    };

    bool initialize_instance(
        const std::vector<const char*>& desired_layers,
        const std::vector<const char*>& desired_extensions);
    bool initialize_physical_device(const std::vector<const char*>& desired_extensions);
    void initialize_logic_device(const std::vector<const char*>& enabled_extensions);
    void initialize_swapchain(const rhi_desc& desc);

    bool check_extension_support(
        const std::vector<const char*>& desired_extensions,
        const std::vector<VkExtensionProperties>& available_extensions) const;

    VkInstance m_instance;
    VkPhysicalDevice m_physical_device;
    VkDevice m_device;

    VkSurfaceKHR m_surface;

    VkSwapchainKHR m_swapchain;
    std::vector<vk_swapchain_image> m_swapchain_images;
    std::uint32_t m_swapchain_image_index;
    VkSemaphore m_swapchain_image_available_semaphore;

    queue_family_indices m_queue_family_indices;
    std::unique_ptr<vk_command_queue> m_graphics_queue;
    std::unique_ptr<vk_command_queue> m_present_queue;

    std::size_t m_frame_count;
    std::size_t m_frame_resource_count;
    std::size_t m_frame_resource_index;

#ifndef NDEUBG
    VkDebugUtilsMessengerEXT m_debug_messenger;
#endif
};
} // namespace violet::vk