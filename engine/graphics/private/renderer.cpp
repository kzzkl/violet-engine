#include "graphics/renderer.hpp"
#include <cassert>

namespace violet
{
rhi_deleter::rhi_deleter() : m_rhi(nullptr)
{
}

rhi_deleter::rhi_deleter(rhi_renderer* rhi) : m_rhi(rhi)
{
}

void rhi_deleter::operator()(rhi_render_pass* render_pass)
{
    m_rhi->destroy_render_pass(render_pass);
}

void rhi_deleter::operator()(rhi_render_pipeline* render_pipeline)
{
    m_rhi->destroy_render_pipeline(render_pipeline);
}

void rhi_deleter::operator()(rhi_compute_pipeline* compute_pipeline)
{
    m_rhi->destroy_compute_pipeline(compute_pipeline);
}

void rhi_deleter::operator()(rhi_parameter_layout* parameter_layout)
{
    m_rhi->destroy_parameter_layout(parameter_layout);
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

void rhi_deleter::operator()(rhi_resource* resource)
{
    m_rhi->destroy_resource(resource);
}

void rhi_deleter::operator()(rhi_fence* fence)
{
    m_rhi->destroy_fence(fence);
}

void rhi_deleter::operator()(rhi_semaphore* semaphore)
{
    m_rhi->destroy_semaphore(semaphore);
}

renderer::renderer(rhi_renderer* rhi) : m_rhi(rhi), m_rhi_deleter(rhi)
{
    add_parameter_layout(
        "violet mesh",
        {
            {RHI_PARAMETER_TYPE_UNIFORM_BUFFER,
             sizeof(float4x4),
             RHI_PARAMETER_FLAG_VERTEX | RHI_PARAMETER_FLAG_FRAGMENT}
    });
    add_parameter_layout(
        "violet camera",
        {
            {RHI_PARAMETER_TYPE_UNIFORM_BUFFER,
             sizeof(float4x4) * 3 + sizeof(float4),
             RHI_PARAMETER_FLAG_VERTEX | RHI_PARAMETER_FLAG_FRAGMENT           },
            {RHI_PARAMETER_TYPE_TEXTURE,        1,  RHI_PARAMETER_FLAG_FRAGMENT}
    });

    rhi_parameter_layout* light_layout = add_parameter_layout(
        "violet light",
        {
            {RHI_PARAMETER_TYPE_UNIFORM_BUFFER, 528, RHI_PARAMETER_FLAG_FRAGMENT}
    });

    m_light_parameter = create_parameter(light_layout);
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

void renderer::present(const std::vector<rhi_semaphore*>& wait_semaphores)
{
    m_rhi->present(wait_semaphores.data(), wait_semaphores.size());
}

void renderer::resize(std::uint32_t width, std::uint32_t height)
{
    m_rhi->resize(width, height);
}

rhi_resource* renderer::get_back_buffer()
{
    return m_rhi->get_back_buffer();
}

rhi_fence* renderer::get_in_flight_fence()
{
    return m_rhi->get_in_flight_fence();
}

rhi_semaphore* renderer::get_image_available_semaphore()
{
    return m_rhi->get_image_available_semaphore();
}

std::size_t renderer::get_frame_resource_count() const noexcept
{
    return m_rhi->get_frame_resource_count();
}

std::size_t renderer::get_frame_resource_index() const noexcept
{
    return m_rhi->get_frame_resource_index();
}

rhi_parameter_layout* renderer::add_parameter_layout(
    std::string_view name,
    const std::vector<rhi_parameter_layout_pair>& layout)
{
    assert(m_parameter_layouts.find(name.data()) == m_parameter_layouts.end());

    rhi_parameter_layout_desc desc = {};
    for (std::size_t i = 0; i < layout.size(); ++i)
        desc.parameters[i] = layout[i];
    desc.parameter_count = layout.size();

    m_parameter_layouts[name.data()] = create_parameter_layout(desc);
    return m_parameter_layouts[name.data()].get();
}

rhi_parameter_layout* renderer::get_parameter_layout(std::string_view name) const
{
    return m_parameter_layouts.at(name.data()).get();
}

rhi_ptr<rhi_render_pass> renderer::create_render_pass(const rhi_render_pass_desc& desc)
{
    return rhi_ptr<rhi_render_pass>(m_rhi->create_render_pass(desc), m_rhi_deleter);
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

rhi_ptr<rhi_parameter_layout> renderer::create_parameter_layout(
    const rhi_parameter_layout_desc& desc)
{
    return rhi_ptr<rhi_parameter_layout>(m_rhi->create_parameter_layout(desc), m_rhi_deleter);
}

rhi_ptr<rhi_parameter> renderer::create_parameter(rhi_parameter_layout* layout)
{
    return rhi_ptr<rhi_parameter>(m_rhi->create_parameter(layout), m_rhi_deleter);
}

rhi_ptr<rhi_framebuffer> renderer::create_framebuffer(const rhi_framebuffer_desc& desc)
{
    return rhi_ptr<rhi_framebuffer>(m_rhi->create_framebuffer(desc), m_rhi_deleter);
}

rhi_ptr<rhi_resource> renderer::create_buffer(const rhi_buffer_desc& desc)
{
    return rhi_ptr<rhi_resource>(m_rhi->create_buffer(desc), m_rhi_deleter);
}

rhi_ptr<rhi_sampler> renderer::create_sampler(const rhi_sampler_desc& desc)
{
    return rhi_ptr<rhi_sampler>(m_rhi->create_sampler(desc), m_rhi_deleter);
}

rhi_ptr<rhi_resource> renderer::create_texture(
    const std::uint8_t* data,
    std::uint32_t width,
    std::uint32_t height,
    rhi_resource_format format)
{
    return rhi_ptr<rhi_resource>(m_rhi->create_texture(data, width, height, format), m_rhi_deleter);
}

rhi_ptr<rhi_resource> renderer::create_texture(const char* file)
{
    return rhi_ptr<rhi_resource>(m_rhi->create_texture(file), m_rhi_deleter);
}

rhi_ptr<rhi_resource> renderer::create_texture_cube(
    std::string_view right,
    std::string_view left,
    std::string_view top,
    std::string_view bottom,
    std::string_view front,
    std::string_view back)
{
    return rhi_ptr<rhi_resource>(
        m_rhi->create_texture_cube(
            right.data(),
            left.data(),
            top.data(),
            bottom.data(),
            front.data(),
            back.data()),
        m_rhi_deleter);
}

rhi_ptr<rhi_resource> renderer::create_render_target(const rhi_render_target_desc& desc)
{
    return rhi_ptr<rhi_resource>(m_rhi->create_render_target(desc), m_rhi_deleter);
}

rhi_ptr<rhi_resource> renderer::create_depth_stencil_buffer(
    const rhi_depth_stencil_buffer_desc& desc)
{
    return rhi_ptr<rhi_resource>(m_rhi->create_depth_stencil_buffer(desc), m_rhi_deleter);
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