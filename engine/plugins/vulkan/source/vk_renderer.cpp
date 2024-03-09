#include "vk_renderer.hpp"
#include "vk_command.hpp"
#include "vk_framebuffer.hpp"
#include "vk_pipeline.hpp"
#include "vk_render_pass.hpp"
#include "vk_swapchain.hpp"
#include "vk_sync.hpp"
#include <algorithm>

namespace violet::vk
{
vk_renderer::vk_renderer() noexcept
{
    m_context = std::make_unique<vk_context>();
}

vk_renderer::~vk_renderer()
{
    vkDeviceWaitIdle(m_context->get_device());

    m_swapchain = nullptr;

    for (frame_resource& frame_resource : m_frame_resources)
        frame_resource.execute_delay_tasks();
    m_frame_resources.clear();
}

bool vk_renderer::initialize(const rhi_desc& desc)
{
    if (!m_context->initialize(desc))
        return false;

    const std::vector<std::uint32_t>& queue_family_indices = {
        m_context->get_graphics_queue()->get_family_index(),
        m_context->get_present_queue()->get_family_index()};
    m_swapchain = std::make_unique<vk_swapchain>(
        desc.width,
        desc.height,
        queue_family_indices,
        m_context.get());

    m_frame_resources.resize(m_context->get_frame_resource_count());
    for (frame_resource& frame_resource : m_frame_resources)
    {
        frame_resource.image_available_semaphore = std::make_unique<vk_semaphore>(m_context.get());
        frame_resource.in_flight_fence = std::make_unique<vk_fence>(true, m_context.get());
    }

    return true;
}

rhi_render_command* vk_renderer::allocate_command()
{
    return m_context->get_graphics_queue()->allocate_command();
}

void vk_renderer::execute(
    rhi_render_command* const* commands,
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

void vk_renderer::begin_frame()
{
    frame_resource& frame_resource = get_current_frame_resource();
    frame_resource.in_flight_fence->wait();

    m_swapchain->acquire_next_image(frame_resource.image_available_semaphore->get_semaphore());

    frame_resource.execute_delay_tasks();

    m_context->get_graphics_queue()->begin_frame();
}

void vk_renderer::end_frame()
{
    m_context->next_frame();
}

void vk_renderer::present(rhi_semaphore* const* wait_semaphores, std::size_t wait_semaphore_count)
{
    m_context->get_present_queue()->present(
        m_swapchain->get_swapchain(),
        m_swapchain->get_image_index(),
        wait_semaphores,
        wait_semaphore_count);
}

void vk_renderer::resize(std::uint32_t width, std::uint32_t height)
{
    vk_check(vkDeviceWaitIdle(m_context->get_device()));
    m_swapchain->resize(width, height);
}

rhi_image* vk_renderer::get_back_buffer()
{
    return m_swapchain->get_current_image();
}

rhi_fence* vk_renderer::get_in_flight_fence()
{
    return m_frame_resources[m_context->get_frame_resource_index()].in_flight_fence.get();
}

rhi_semaphore* vk_renderer::get_image_available_semaphore()
{
    return m_frame_resources[m_context->get_frame_resource_index()].image_available_semaphore.get();
}

rhi_render_pass* vk_renderer::create_render_pass(const rhi_render_pass_desc& desc)
{
    return new vk_render_pass(desc, m_context.get());
}

void vk_renderer::destroy_render_pass(rhi_render_pass* render_pass)
{
    delay_delete(render_pass);
}

rhi_render_pipeline* vk_renderer::create_render_pipeline(const rhi_render_pipeline_desc& desc)
{
    return new vk_render_pipeline(desc, VkExtent2D{128, 128}, m_context.get());
}

void vk_renderer::destroy_render_pipeline(rhi_render_pipeline* render_pipeline)
{
    delay_delete(render_pipeline);
}

rhi_compute_pipeline* vk_renderer::create_compute_pipeline(const rhi_compute_pipeline_desc& desc)
{
    return new vk_compute_pipeline(desc, m_context.get());
}

void vk_renderer::destroy_compute_pipeline(rhi_compute_pipeline* compute_pipeline)
{
    delay_delete(compute_pipeline);
}

rhi_parameter_layout* vk_renderer::create_parameter_layout(const rhi_parameter_layout_desc& desc)
{
    return new vk_parameter_layout(desc, m_context.get());
}

void vk_renderer::destroy_parameter_layout(rhi_parameter_layout* parameter_layout)
{
    delete parameter_layout;
}

rhi_parameter* vk_renderer::create_parameter(rhi_parameter_layout* layout)
{
    return new vk_parameter(static_cast<vk_parameter_layout*>(layout), m_context.get());
}

void vk_renderer::destroy_parameter(rhi_parameter* parameter)
{
    delay_delete(parameter);
}

rhi_framebuffer* vk_renderer::create_framebuffer(const rhi_framebuffer_desc& desc)
{
    return new vk_framebuffer(desc, m_context.get());
}

void vk_renderer::destroy_framebuffer(rhi_framebuffer* framebuffer)
{
    delay_delete(framebuffer);
}

rhi_sampler* vk_renderer::create_sampler(const rhi_sampler_desc& desc)
{
    return new vk_sampler(desc, m_context.get());
}

void vk_renderer::destroy_sampler(rhi_sampler* sampler)
{
    delay_delete(sampler);
}

rhi_buffer* vk_renderer::create_buffer(const rhi_buffer_desc& desc)
{
    if (desc.flags & RHI_BUFFER_FLAG_VERTEX)
        return new vk_vertex_buffer(desc, m_context.get());
    else if (desc.flags & RHI_BUFFER_FLAG_INDEX)
        return new vk_index_buffer(desc, m_context.get());
    else if (desc.flags & RHI_BUFFER_FLAG_STORAGE)
        return new vk_storage_buffer(desc, m_context.get());
    else
        return nullptr;
}

void vk_renderer::destroy_buffer(rhi_buffer* buffer)
{
    delay_delete(buffer);
}

rhi_image* vk_renderer::create_image(const rhi_image_desc& desc)
{
    return new vk_image(desc, m_context.get());
}

rhi_image* vk_renderer::create_image(const char* file, const rhi_image_desc& desc)
{
    return new vk_texture(file, desc, m_context.get());
}

rhi_image* vk_renderer::create_image_cube(const rhi_image_desc& desc)
{
    return nullptr;
}

rhi_image* vk_renderer::create_image_cube(
    const char* right,
    const char* left,
    const char* top,
    const char* bottom,
    const char* front,
    const char* back,
    const rhi_image_desc& desc)
{
    return new vk_texture_cube(right, left, top, bottom, front, back, desc, m_context.get());
}

void vk_renderer::destroy_image(rhi_image* image)
{
    delay_delete(image);
}

rhi_fence* vk_renderer::create_fence(bool signaled)
{
    return new vk_fence(signaled, m_context.get());
}

void vk_renderer::destroy_fence(rhi_fence* fence)
{
    delay_delete(fence);
}

rhi_semaphore* vk_renderer::create_semaphore()
{
    return new vk_semaphore(m_context.get());
}

void vk_renderer::destroy_semaphore(rhi_semaphore* semaphore)
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

    PLUGIN_API violet::rhi_renderer* create_rhi()
    {
        return new violet::vk::vk_renderer();
    }

    PLUGIN_API void destroy_rhi(violet::rhi_renderer* rhi)
    {
        delete rhi;
    }
}