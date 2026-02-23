#include "vk_rhi.hpp"
#include "core/plugin_interface.hpp"
#include "vk_command.hpp"
#include "vk_framebuffer.hpp"
#include "vk_pipeline.hpp"
#include "vk_render_pass.hpp"
#include "vk_swapchain.hpp"
#include "vk_sync.hpp"

namespace violet::vk
{
vk_rhi::vk_rhi() noexcept
{
    m_context = std::make_unique<vk_context>();
}

vk_rhi::~vk_rhi()
{
    vkDeviceWaitIdle(m_context->get_device());
    m_context->get_deletion_queue()->flush();
}

bool vk_rhi::initialize(const rhi_desc& desc)
{
    return m_context->initialize(desc);
}

rhi_command* vk_rhi::allocate_command()
{
    return m_context->get_graphics_queue()->allocate_command();
}

void vk_rhi::execute(rhi_command_batch* batchs, std::uint32_t batch_count)
{
    m_context->get_graphics_queue()->execute(batchs, batch_count);
}

void vk_rhi::execute_sync(rhi_command* command)
{
    m_context->get_graphics_queue()->execute_sync(command);
}

void vk_rhi::begin_frame()
{
    m_context->begin_frame();
}

void vk_rhi::end_frame()
{
    m_context->end_frame();
}

rhi_render_pass* vk_rhi::create_render_pass(const rhi_render_pass_desc& desc)
{
    return new vk_render_pass(desc, m_context.get());
}

void vk_rhi::destroy_render_pass(rhi_render_pass* render_pass)
{
    m_context->get_deletion_queue()->push(render_pass);
}

rhi_shader* vk_rhi::create_shader(const rhi_shader_desc& desc)
{
    switch (desc.stage)
    {
    case RHI_SHADER_STAGE_VERTEX:
        return new vk_vertex_shader(desc, m_context.get());
    case RHI_SHADER_STAGE_GEOMETRY:
        return new vk_geometry_shader(desc, m_context.get());
    case RHI_SHADER_STAGE_FRAGMENT:
        return new vk_fragment_shader(desc, m_context.get());
    case RHI_SHADER_STAGE_COMPUTE:
        return new vk_compute_shader(desc, m_context.get());
    default:
        throw std::runtime_error("Invalid shader stage.");
    }
}

void vk_rhi::destroy_shader(rhi_shader* shader)
{
    delete shader;
}

rhi_raster_pipeline* vk_rhi::create_raster_pipeline(const rhi_raster_pipeline_desc& desc)
{
    return new vk_raster_pipeline(desc, m_context.get());
}

void vk_rhi::destroy_raster_pipeline(rhi_raster_pipeline* raster_pipeline)
{
    m_context->get_deletion_queue()->push(raster_pipeline);
}

rhi_compute_pipeline* vk_rhi::create_compute_pipeline(const rhi_compute_pipeline_desc& desc)
{
    return new vk_compute_pipeline(desc, m_context.get());
}

void vk_rhi::destroy_compute_pipeline(rhi_compute_pipeline* compute_pipeline)
{
    m_context->get_deletion_queue()->push(compute_pipeline);
}

rhi_parameter* vk_rhi::create_parameter(const rhi_parameter_desc& desc, bool auto_sync)
{
    return new vk_parameter(desc, m_context.get());
}

void vk_rhi::destroy_parameter(rhi_parameter* parameter)
{
    m_context->get_deletion_queue()->push(parameter);
}

rhi_sampler* vk_rhi::create_sampler(const rhi_sampler_desc& desc)
{
    return new vk_sampler(desc, m_context.get());
}

void vk_rhi::destroy_sampler(rhi_sampler* sampler)
{
    m_context->get_deletion_queue()->push(sampler);
}

rhi_buffer* vk_rhi::create_buffer(const rhi_buffer_desc& desc)
{
    return new vk_buffer(desc, m_context.get());
}

void vk_rhi::destroy_buffer(rhi_buffer* buffer)
{
    m_context->get_deletion_queue()->push(buffer);
}

rhi_texture* vk_rhi::create_texture(const rhi_texture_desc& desc)
{
    return new vk_texture(desc, m_context.get());
}

void vk_rhi::destroy_texture(rhi_texture* texture)
{
    m_context->get_deletion_queue()->push(texture);
}

rhi_swapchain* vk_rhi::create_swapchain(const rhi_swapchain_desc& desc)
{
    return new vk_swapchain(desc, m_context.get());
}

void vk_rhi::destroy_swapchain(rhi_swapchain* swapchain)
{
    m_context->get_deletion_queue()->push(swapchain);
}

rhi_fence* vk_rhi::create_fence()
{
    return new vk_fence(true, m_context.get());
}

void vk_rhi::destroy_fence(rhi_fence* fence)
{
    m_context->get_deletion_queue()->push(fence);
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