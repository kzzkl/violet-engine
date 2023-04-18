#pragma once

#include "vk_common.hpp"
#include "vk_framebuffer_cache.hpp"
#include "vk_resource.hpp"
#include <memory>

namespace violet::vk
{
class vk_rhi : public rhi_interface
{
public:
    vk_rhi() noexcept;
    vk_rhi(const vk_rhi&) = delete;
    virtual ~vk_rhi();

    virtual bool initialize(const rhi_desc& desc) override;

    virtual render_command_interface* allocate_command() override { return nullptr; }
    virtual void execute(render_command_interface* command) override {}

    virtual void present() override;
    virtual void resize(std::uint32_t width, std::uint32_t height) override {}

    virtual resource_interface* get_back_buffer() override { return nullptr; }

    vk_rhi& operator=(const vk_rhi&) = delete;

public:
    virtual render_pipeline_interface* make_render_pipeline(
        const render_pipeline_desc& desc) override;

    virtual pipeline_parameter_interface* make_pipeline_parameter(
        const pipeline_parameter_desc& desc) override;

    virtual resource_interface* make_vertex_buffer(const vertex_buffer_desc& desc) override;
    virtual resource_interface* make_index_buffer(const index_buffer_desc& desc) override;

    virtual resource_interface* make_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height,
        resource_format format = RESOURCE_FORMAT_R8G8B8A8_UNORM) override;
    virtual resource_interface* make_texture(const char* file) override;
    virtual resource_interface* make_texture_cube(
        const char* left,
        const char* right,
        const char* top,
        const char* bottom,
        const char* front,
        const char* back) override;

    virtual resource_interface* make_shadow_map(const shadow_map_desc& desc) override;

    virtual resource_interface* make_render_target(const render_target_desc& desc) override;
    virtual resource_interface* make_depth_stencil_buffer(
        const depth_stencil_buffer_desc& desc) override;

public:
    VkDevice get_device() const noexcept { return m_device; }
    vk_framebuffer_cache* get_framebuffer_cache() const noexcept
    {
        return m_framebuffer_cache.get();
    }

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

    VkQueue m_graphics_queue;
    VkQueue m_present_queue;
    queue_family_indices m_queue_family_indices;

    std::unique_ptr<vk_framebuffer_cache> m_framebuffer_cache;

    std::size_t m_frame_count;
    std::size_t m_frame_resource_count;
    std::size_t m_frame_resource_index;

#ifndef NDEUBG
    VkDebugUtilsMessengerEXT m_debug_messenger;
#endif
};
} // namespace violet::vk