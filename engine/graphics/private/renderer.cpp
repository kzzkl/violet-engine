#include "graphics/renderer.hpp"
#include "math/math.hpp"

namespace violet
{
rhi_deleter::rhi_deleter() : m_rhi(nullptr)
{
}

rhi_deleter::rhi_deleter(rhi* rhi) : m_rhi(rhi)
{
}

void rhi_deleter::operator()(rhi_render_pass* render_pass)
{
    m_rhi->destroy_render_pass(render_pass);
}

void rhi_deleter::operator()(rhi_shader* shader)
{
    m_rhi->destroy_shader(shader);
}

void rhi_deleter::operator()(rhi_render_pipeline* render_pipeline)
{
    m_rhi->destroy_render_pipeline(render_pipeline);
}

void rhi_deleter::operator()(rhi_compute_pipeline* compute_pipeline)
{
    m_rhi->destroy_compute_pipeline(compute_pipeline);
}

void rhi_deleter::operator()(rhi_parameter* parameter)
{
    m_rhi->destroy_parameter(parameter);
}

void rhi_deleter::operator()(rhi_framebuffer* framebuffer)
{
    m_rhi->destroy_framebuffer(framebuffer);
}

void rhi_deleter::operator()(rhi_sampler* sampler)
{
    m_rhi->destroy_sampler(sampler);
}

void rhi_deleter::operator()(rhi_buffer* buffer)
{
    m_rhi->destroy_buffer(buffer);
}

void rhi_deleter::operator()(rhi_texture* texture)
{
    m_rhi->destroy_texture(texture);
}

void rhi_deleter::operator()(rhi_swapchain* swapchain)
{
    m_rhi->destroy_swapchain(swapchain);
}

void rhi_deleter::operator()(rhi_fence* fence)
{
    m_rhi->destroy_fence(fence);
}

void rhi_deleter::operator()(rhi_semaphore* semaphore)
{
    m_rhi->destroy_semaphore(semaphore);
}

renderer::renderer(rhi* rhi) : m_rhi(rhi), m_rhi_deleter(rhi)
{
    /*add_parameter_layout(
        "violet mesh",
        {
            {RHI_PARAMETER_TYPE_UNIFORM_BUFFER,
             sizeof(float4x4),
             RHI_PARAMETER_STAGE_FLAG_VERTEX | RHI_PARAMETER_STAGE_FLAG_FRAGMENT}
    });
    add_parameter_layout(
        "violet camera",
        {
            {RHI_PARAMETER_TYPE_UNIFORM_BUFFER,
             sizeof(float4x4) * 3 + sizeof(float4),
             RHI_PARAMETER_STAGE_FLAG_VERTEX | RHI_PARAMETER_STAGE_FLAG_FRAGMENT     },
            {RHI_PARAMETER_TYPE_TEXTURE,        1,  RHI_PARAMETER_STAGE_FLAG_FRAGMENT}
    });
    add_parameter_layout(
        "violet light",
        {
            {RHI_PARAMETER_TYPE_UNIFORM_BUFFER, 528, RHI_PARAMETER_STAGE_FLAG_FRAGMENT}
    });*/
}

renderer::~renderer()
{
}

rhi_render_command* renderer::allocate_command()
{
    return m_rhi->allocate_command();
}

void renderer::execute(
    const std::vector<rhi_render_command*>& commands,
    const std::vector<rhi_semaphore*>& signal_semaphores,
    const std::vector<rhi_semaphore*>& wait_semaphores,
    rhi_fence* fence)
{
    m_rhi->execute(
        commands.data(),
        commands.size(),
        signal_semaphores.data(),
        signal_semaphores.size(),
        wait_semaphores.data(),
        wait_semaphores.size(),
        fence);
}

void renderer::begin_frame()
{
    m_rhi->begin_frame();
}

void renderer::end_frame()
{
    m_rhi->end_frame();
}

rhi_fence* renderer::get_in_flight_fence()
{
    return m_rhi->get_in_flight_fence();
}

std::size_t renderer::get_frame_resource_count() const noexcept
{
    return m_rhi->get_frame_resource_count();
}

std::size_t renderer::get_frame_resource_index() const noexcept
{
    return m_rhi->get_frame_resource_index();
}

rhi_ptr<rhi_render_pass> renderer::create_render_pass(const rhi_render_pass_desc& desc)
{
    return rhi_ptr<rhi_render_pass>(m_rhi->create_render_pass(desc), m_rhi_deleter);
}

rhi_ptr<rhi_shader> renderer::create_shader(const char* file)
{
    return rhi_ptr<rhi_shader>(m_rhi->create_shader(file), m_rhi_deleter);
}

rhi_ptr<rhi_render_pipeline> renderer::create_render_pipeline(const rhi_render_pipeline_desc& desc)
{
    return rhi_ptr<rhi_render_pipeline>(m_rhi->create_render_pipeline(desc), m_rhi_deleter);
}

rhi_ptr<rhi_compute_pipeline> renderer::create_compute_pipeline(
    const rhi_compute_pipeline_desc& desc)
{
    return rhi_ptr<rhi_compute_pipeline>(m_rhi->create_compute_pipeline(desc), m_rhi_deleter);
}

rhi_ptr<rhi_parameter> renderer::create_parameter(const rhi_parameter_desc& desc)
{
    return rhi_ptr<rhi_parameter>(m_rhi->create_parameter(desc), m_rhi_deleter);
}

rhi_ptr<rhi_framebuffer> renderer::create_framebuffer(const rhi_framebuffer_desc& desc)
{
    return rhi_ptr<rhi_framebuffer>(m_rhi->create_framebuffer(desc), m_rhi_deleter);
}

rhi_ptr<rhi_buffer> renderer::create_buffer(const rhi_buffer_desc& desc)
{
    return rhi_ptr<rhi_buffer>(m_rhi->create_buffer(desc), m_rhi_deleter);
}

rhi_ptr<rhi_sampler> renderer::create_sampler(const rhi_sampler_desc& desc)
{
    return rhi_ptr<rhi_sampler>(m_rhi->create_sampler(desc), m_rhi_deleter);
}

rhi_ptr<rhi_texture> renderer::create_texture(const rhi_texture_desc& desc)
{
    return rhi_ptr<rhi_texture>(m_rhi->create_texture(desc), m_rhi_deleter);
}

rhi_ptr<rhi_texture> renderer::create_texture(const char* file, const rhi_texture_desc& desc)
{
    return rhi_ptr<rhi_texture>(m_rhi->create_texture(file, desc), m_rhi_deleter);
}

rhi_ptr<rhi_texture> renderer::create_texture_cube(
    std::string_view right,
    std::string_view left,
    std::string_view top,
    std::string_view bottom,
    std::string_view front,
    std::string_view back,
    const rhi_texture_desc& desc)
{
    return rhi_ptr<rhi_texture>(
        m_rhi->create_texture_cube(
            right.data(),
            left.data(),
            top.data(),
            bottom.data(),
            front.data(),
            back.data(),
            desc),
        m_rhi_deleter);
}

rhi_ptr<rhi_swapchain> renderer::create_swapchain(const rhi_swapchain_desc& desc)
{
    return rhi_ptr<rhi_swapchain>(m_rhi->create_swapchain(desc), m_rhi_deleter);
}

rhi_ptr<rhi_fence> renderer::create_fence(bool signaled)
{
    return rhi_ptr<rhi_fence>(m_rhi->create_fence(signaled), m_rhi_deleter);
}

rhi_ptr<rhi_semaphore> renderer::create_semaphore()
{
    return rhi_ptr<rhi_semaphore>(m_rhi->create_semaphore(), m_rhi_deleter);
}
} // namespace violet