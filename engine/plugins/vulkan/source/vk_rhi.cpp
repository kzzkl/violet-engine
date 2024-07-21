#include "vk_rhi.hpp"
#include "vk_command.hpp"
#include "vk_framebuffer.hpp"
#include "vk_pipeline.hpp"
#include "vk_render_pass.hpp"
#include "vk_swapchain.hpp"
#include "vk_sync.hpp"
#include <algorithm>

namespace violet::vk
{
vk_rhi::vk_rhi() noexcept
{
    m_context = std::make_unique<vk_context>();
}

vk_rhi::~vk_rhi()
{
    vkDeviceWaitIdle(m_context->get_device());

    for (frame_resource& frame_resource : m_frame_resources)
        frame_resource.execute_delay_tasks();
    m_frame_resources.clear();
}

bool vk_rhi::initialize(const rhi_desc& desc)
{
    if (!m_context->initialize(desc))
        return false;

    m_frame_resources.resize(m_context->get_frame_resource_count());
    for (frame_resource& frame_resource : m_frame_resources)
        frame_resource.in_flight_fence = std::make_unique<vk_fence>(true, m_context.get());

    return true;
}

rhi_command* vk_rhi::allocate_command()
{
    return m_context->get_graphics_queue()->allocate_command();
}

void vk_rhi::execute(
    rhi_command* const* commands,
    std::size_t command_count,
    rhi_semaphore* const* signal_semaphores,
    std::size_t signal_semaphore_count,
    rhi_semaphore* const* wait_semaphores,
    std::size_t wait_semaphore_count,
    rhi_fence* fence)
{
    m_context->get_graphics_queue()->execute(
        commands,
        command_count,
        signal_semaphores,
        signal_semaphore_count,
        wait_semaphores,
        wait_semaphore_count,
        fence);
}

void vk_rhi::begin_frame()
{
    frame_resource& frame_resource = get_current_frame_resource();
    frame_resource.in_flight_fence->wait();

    frame_resource.execute_delay_tasks();

    m_context->get_graphics_queue()->begin_frame();
}

void vk_rhi::end_frame()
{
    m_context->next_frame();
}

rhi_fence* vk_rhi::get_in_flight_fence()
{
    return m_frame_resources[m_context->get_frame_resource_index()].in_flight_fence.get();
}

rhi_render_pass* vk_rhi::create_render_pass(const rhi_render_pass_desc& desc)
{
    return new vk_render_pass(desc, m_context.get());
}

void vk_rhi::destroy_render_pass(rhi_render_pass* render_pass)
{
    delay_delete(render_pass);
}

rhi_shader* vk_rhi::create_shader(const rhi_shader_desc& desc)
{
    if (desc.stage == RHI_SHADER_STAGE_VERTEX)
        return new vk_vertex_shader(desc, m_context.get());
    else if (desc.stage == RHI_SHADER_STAGE_FRAGMENT)
        return new vk_fragment_shader(desc, m_context.get());
    else if (desc.stage == RHI_SHADER_STAGE_COMPUTE)
        return new vk_compute_shader(desc, m_context.get());
    else
        throw vk_exception("Invalid shader stage.");
}

void vk_rhi::destroy_shader(rhi_shader* shader)
{
    delete shader;
}

rhi_render_pipeline* vk_rhi::create_render_pipeline(const rhi_render_pipeline_desc& desc)
{
    return new vk_render_pipeline(desc, m_context.get());
}

void vk_rhi::destroy_render_pipeline(rhi_render_pipeline* render_pipeline)
{
    delay_delete(render_pipeline);
}

rhi_compute_pipeline* vk_rhi::create_compute_pipeline(const rhi_compute_pipeline_desc& desc)
{
    return new vk_compute_pipeline(desc, m_context.get());
}

void vk_rhi::destroy_compute_pipeline(rhi_compute_pipeline* compute_pipeline)
{
    delay_delete(compute_pipeline);
}

rhi_parameter* vk_rhi::create_parameter(const rhi_parameter_desc& desc)
{
    return new vk_parameter(desc, m_context.get());
}

void vk_rhi::destroy_parameter(rhi_parameter* parameter)
{
    delay_delete(parameter);
}

rhi_framebuffer* vk_rhi::create_framebuffer(const rhi_framebuffer_desc& desc)
{
    return new vk_framebuffer(desc, m_context.get());
}

void vk_rhi::destroy_framebuffer(rhi_framebuffer* framebuffer)
{
    delay_delete(framebuffer);
}

rhi_sampler* vk_rhi::create_sampler(const rhi_sampler_desc& desc)
{
    return new vk_sampler(desc, m_context.get());
}

void vk_rhi::destroy_sampler(rhi_sampler* sampler)
{
    delay_delete(sampler);
}

rhi_buffer* vk_rhi::create_buffer(const rhi_buffer_desc& desc)
{
    if (desc.flags & RHI_BUFFER_INDEX)
        return new vk_index_buffer(desc, m_context.get());
    else
        return new vk_buffer(desc, m_context.get());
}

void vk_rhi::destroy_buffer(rhi_buffer* buffer)
{
    delay_delete(buffer);
}

rhi_texture* vk_rhi::create_texture(const rhi_texture_desc& desc)
{
    return new vk_texture(desc, m_context.get());
}

rhi_texture* vk_rhi::create_texture_view(const rhi_texture_view_desc& desc)
{
    return new vk_texture_view(desc, m_context.get());
}

void vk_rhi::destroy_texture(rhi_texture* texture)
{
    delay_delete(texture);
}

rhi_swapchain* vk_rhi::create_swapchain(const rhi_swapchain_desc& desc)
{
    return new vk_swapchain(desc, m_context.get());
}

void vk_rhi::destroy_swapchain(rhi_swapchain* swapchain)
{
    delay_delete(swapchain);
}

rhi_fence* vk_rhi::create_fence(bool signaled)
{
    return new vk_fence(signaled, m_context.get());
}

void vk_rhi::destroy_fence(rhi_fence* fence)
{
    delay_delete(fence);
}

rhi_semaphore* vk_rhi::create_semaphore()
{
    return new vk_semaphore(m_context.get());
}

void vk_rhi::destroy_semaphore(rhi_semaphore* semaphore)
{
    delay_delete(semaphore);
}
} // namespace violet::vk

extern "C"
{
    PLUGIN_API violet::plugin_info get_plugin_info()
    {
        violet::plugin_info info = {};

        char name[] = "graphics-vk";
        memcpy(info.name, name, sizeof(name));

        info.version.major = 1;
        info.version.minor = 0;

        return info;
    }

    PLUGIN_API violet::rhi* create_rhi()
    {
        return new violet::vk::vk_rhi();
    }

    PLUGIN_API void destroy_rhi(violet::rhi* rhi)
    {
        delete rhi;
    }
}