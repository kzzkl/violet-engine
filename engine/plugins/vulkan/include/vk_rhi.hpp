#pragma once

#include "vk_common.hpp"
#include "vk_context.hpp"
#include "vk_swapchain.hpp"
#include "vk_sync.hpp"
#include <functional>

namespace violet::vk
{
class vk_rhi : public rhi
{
public:
    vk_rhi() noexcept;
    vk_rhi(const vk_rhi&) = delete;
    virtual ~vk_rhi();

    bool initialize(const rhi_desc& desc) override;

    rhi_command* allocate_command() override;
    void execute(
        rhi_command* const* commands,
        std::size_t command_count,
        rhi_semaphore* const* signal_semaphores,
        std::size_t signal_semaphore_count,
        rhi_semaphore* const* wait_semaphores,
        std::size_t wait_semaphore_count,
        rhi_fence* fence) override;

    void begin_frame() override;
    void end_frame() override;

    rhi_fence* get_in_flight_fence() override;

    std::size_t get_frame_count() const noexcept override
    {
        return m_context->get_frame_count();
    }

    std::size_t get_frame_resource_count() const noexcept override
    {
        return m_context->get_frame_resource_count();
    }

    std::size_t get_frame_resource_index() const noexcept override
    {
        return m_context->get_frame_resource_index();
    }

    template <typename T>
    void delay_delete(T* object)
    {
        frame_resource& frame_resource = get_current_frame_resource();
        frame_resource.delay_tasks.push_back(
            [object]()
            {
                delete object;
            });
    }

    vk_rhi& operator=(const vk_rhi&) = delete;

public:
    void set_name(rhi_texture* object, const char* name) const override
    {
        set_name(static_cast<vk_texture*>(object)->get_image(), VK_OBJECT_TYPE_IMAGE, name);
        set_name(static_cast<vk_texture*>(object)->get_image_view(), VK_OBJECT_TYPE_IMAGE_VIEW, name);
    }

    void set_name(rhi_buffer* object, const char* name) const override
    {
        set_name(static_cast<vk_buffer*>(object)->get_buffer_handle(), VK_OBJECT_TYPE_BUFFER, name);
    }

public:
    rhi_render_pass* create_render_pass(const rhi_render_pass_desc& desc) override;
    void destroy_render_pass(rhi_render_pass* render_pass) override;

    rhi_shader* create_shader(const rhi_shader_desc& desc) override;
    void destroy_shader(rhi_shader* shader) override;

    rhi_render_pipeline* create_render_pipeline(const rhi_render_pipeline_desc& desc) override;
    void destroy_render_pipeline(rhi_render_pipeline* render_pipeline) override;

    rhi_compute_pipeline* create_compute_pipeline(const rhi_compute_pipeline_desc& desc) override;
    void destroy_compute_pipeline(rhi_compute_pipeline* compute_pipeline) override;

    rhi_parameter* create_parameter(const rhi_parameter_desc& desc) override;
    void destroy_parameter(rhi_parameter* parameter) override;

    rhi_framebuffer* create_framebuffer(const rhi_framebuffer_desc& desc) override;
    void destroy_framebuffer(rhi_framebuffer* framebuffer) override;

    rhi_sampler* create_sampler(const rhi_sampler_desc& desc) override;
    void destroy_sampler(rhi_sampler* sampler) override;

    rhi_buffer* create_buffer(const rhi_buffer_desc& desc) override;
    void destroy_buffer(rhi_buffer* buffer) override;

    rhi_texture* create_texture(const rhi_texture_desc& desc) override;
    rhi_texture* create_texture_view(const rhi_texture_view_desc& desc) override;
    void destroy_texture(rhi_texture* texture) override;

    rhi_swapchain* create_swapchain(const rhi_swapchain_desc& desc) override;
    void destroy_swapchain(rhi_swapchain* swapchain) override;

    rhi_fence* create_fence(bool signaled) override;
    void destroy_fence(rhi_fence* fence) override;

    rhi_semaphore* create_semaphore() override;
    void destroy_semaphore(rhi_semaphore* semaphore) override;

private:
    struct frame_resource
    {
        void execute_delay_tasks()
        {
            for (auto& task : delay_tasks)
                task();
            delay_tasks.clear();
        }

        std::unique_ptr<vk_fence> in_flight_fence;

        std::vector<std::function<void()>> delay_tasks;
    };

    frame_resource& get_current_frame_resource()
    {
        return m_frame_resources[m_context->get_frame_resource_index()];
    }

    template <typename T>
    void set_name(T object, VkObjectType type, const char* name) const
    {
        VkDebugUtilsObjectNameInfoEXT info = {};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        info.objectType = type;
        info.objectHandle = reinterpret_cast<std::uint64_t>(object);
        info.pObjectName = name;
        vkSetDebugUtilsObjectNameEXT(m_context->get_device(), &info);
    }

    std::unique_ptr<vk_context> m_context;
    std::vector<frame_resource> m_frame_resources;
};
} // namespace violet::vk