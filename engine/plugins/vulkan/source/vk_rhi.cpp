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

    for (frame_resource& frame_resource : m_frame_resources)
    {
        frame_resource.execute_delay_tasks();
    }
    m_frame_resources.clear();
}

bool vk_rhi::initialize(const rhi_desc& desc)
{
    if (!m_context->initialize(desc))
    {
        return false;
    }

    m_frame_resources.resize(m_context->get_frame_resource_count());

    std::vector<rhi_parameter_binding> bindless_parameter_bindings = {
        {
            .type = RHI_PARAMETER_BINDING_MUTABLE,
            .stages = RHI_SHADER_STAGE_ALL,
            .size = 0,
        },
        {
            .type = RHI_PARAMETER_BINDING_SAMPLER,
            .stages = RHI_SHADER_STAGE_FRAGMENT | RHI_SHADER_STAGE_COMPUTE,
            .size = 128,
        },
    };

    rhi_parameter_desc bindless_parameter_desc = {
        .bindings = bindless_parameter_bindings.data(),
        .binding_count = static_cast<std::uint32_t>(bindless_parameter_bindings.size()),
        .flags = RHI_PARAMETER_SIMPLE | RHI_PARAMETER_DISABLE_SYNC,
    };
    m_bindless_parameter = std::make_unique<vk_parameter>(bindless_parameter_desc, m_context.get());

    return true;
}

rhi_command* vk_rhi::allocate_command()
{
    return m_context->get_graphics_queue()->allocate_command();
}

void vk_rhi::execute(rhi_command* command)
{
    m_context->get_graphics_queue()->execute(command);
}

void vk_rhi::begin_frame()
{
    frame_resource& frame_resource = get_current_frame_resource();
    frame_resource.execute_delay_tasks();

    m_context->get_graphics_queue()->begin_frame();
}

void vk_rhi::end_frame()
{
    m_context->next_frame();
}

rhi_render_pass* vk_rhi::create_render_pass(const rhi_render_pass_desc& desc)
{
    return new vk_render_pass(desc, m_context.get());
}

void vk_rhi::destroy_render_pass(rhi_render_pass* render_pass)
{
    add_delay_delete(render_pass);
}

rhi_shader* vk_rhi::create_shader(const rhi_shader_desc& desc)
{
    switch (desc.stage)
    {
    case RHI_SHADER_STAGE_VERTEX:
        return new vk_vertex_shader(desc, m_context.get());
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

rhi_render_pipeline* vk_rhi::create_render_pipeline(const rhi_render_pipeline_desc& desc)
{
    return new vk_render_pipeline(desc, m_context.get());
}

void vk_rhi::destroy_render_pipeline(rhi_render_pipeline* render_pipeline)
{
    add_delay_delete(render_pipeline);
}

rhi_compute_pipeline* vk_rhi::create_compute_pipeline(const rhi_compute_pipeline_desc& desc)
{
    return new vk_compute_pipeline(desc, m_context.get());
}

void vk_rhi::destroy_compute_pipeline(rhi_compute_pipeline* compute_pipeline)
{
    add_delay_delete(compute_pipeline);
}

rhi_parameter* vk_rhi::create_parameter(const rhi_parameter_desc& desc, bool auto_sync)
{
    return new vk_parameter(desc, m_context.get());
}

void vk_rhi::destroy_parameter(rhi_parameter* parameter)
{
    add_delay_delete(parameter);
}

rhi_framebuffer* vk_rhi::create_framebuffer(const rhi_framebuffer_desc& desc)
{
    return new vk_framebuffer(desc, m_context.get());
}

void vk_rhi::destroy_framebuffer(rhi_framebuffer* framebuffer)
{
    add_delay_delete(framebuffer);
}

rhi_sampler* vk_rhi::create_sampler(const rhi_sampler_desc& desc)
{
    vk_sampler* sampler = new vk_sampler(desc, m_context.get());

    std::lock_guard lock(m_sampler_allocator_mutex);

    rhi_resource_handle handle = m_sampler_allocator.allocate();
    m_bindless_parameter->set_sampler(1, sampler, handle);

    sampler->set_handle(handle);

    return sampler;
}

void vk_rhi::destroy_sampler(rhi_sampler* sampler)
{
    add_delay_task(
        [sampler, this]()
        {
            std::lock_guard lock(m_resource_allocator_mutex);
            m_resource_allocator.free(sampler->get_handle());

            delete sampler;
        });
}

rhi_buffer* vk_rhi::create_buffer(const rhi_buffer_desc& desc)
{
    vk_buffer* buffer = nullptr;

    if (desc.flags & RHI_BUFFER_INDEX)
    {
        buffer = new vk_index_buffer(desc, m_context.get());
    }
    else if ((desc.flags & RHI_BUFFER_UNIFORM_TEXEL) || (desc.flags & RHI_BUFFER_STORAGE_TEXEL))
    {
        buffer = new vk_texel_buffer(desc, m_context.get());
    }
    else
    {
        buffer = new vk_buffer(desc, m_context.get());
    }

    if ((desc.flags & RHI_BUFFER_UNIFORM) || (desc.flags & RHI_BUFFER_UNIFORM_TEXEL))
    {
        std::lock_guard lock(m_resource_allocator_mutex);

        rhi_resource_handle handle = m_resource_allocator.allocate();
        m_bindless_parameter->set_uniform(0, buffer, handle);

        buffer->set_handle(handle);
    }
    else if ((desc.flags & RHI_BUFFER_STORAGE) || (desc.flags & RHI_BUFFER_STORAGE_TEXEL))
    {
        std::lock_guard lock(m_resource_allocator_mutex);

        rhi_resource_handle handle = m_resource_allocator.allocate();
        m_bindless_parameter->set_storage(0, buffer, handle);

        buffer->set_handle(handle);
    }

    return buffer;
}

void vk_rhi::destroy_buffer(rhi_buffer* buffer)
{
    add_delay_task(
        [buffer, this]()
        {
            if (buffer->get_handle() != RHI_INVALID_RESOURCE_HANDLE)
            {
                std::lock_guard lock(m_resource_allocator_mutex);
                m_resource_allocator.free(buffer->get_handle());
            }

            delete buffer;
        });
}

rhi_texture* vk_rhi::create_texture(const rhi_texture_desc& desc)
{
    vk_texture* texture = new vk_texture(desc, m_context.get());

    if (desc.flags & RHI_TEXTURE_SHADER_RESOURCE)
    {
        std::lock_guard lock(m_resource_allocator_mutex);

        rhi_resource_handle handle = m_resource_allocator.allocate();
        m_bindless_parameter->set_texture(0, texture, handle);

        texture->set_handle(handle);
    }

    return texture;
}

rhi_texture* vk_rhi::create_texture(const rhi_texture_view_desc& desc)
{
    vk_texture_view* texture = new vk_texture_view(desc, m_context.get());

    if (desc.texture->get_handle() != RHI_INVALID_RESOURCE_HANDLE)
    {
        std::lock_guard lock(m_resource_allocator_mutex);

        rhi_resource_handle handle = m_resource_allocator.allocate();
        m_bindless_parameter->set_texture(0, texture, handle);

        texture->set_handle(handle);
    }

    return texture;
}

void vk_rhi::destroy_texture(rhi_texture* texture)
{
    add_delay_task(
        [texture, this]()
        {
            if (texture->get_handle() != RHI_INVALID_RESOURCE_HANDLE)
            {
                std::lock_guard lock(m_resource_allocator_mutex);
                m_resource_allocator.free(texture->get_handle());
            }

            delete texture;
        });
}

rhi_swapchain* vk_rhi::create_swapchain(const rhi_swapchain_desc& desc)
{
    return new vk_swapchain(desc, m_context.get());
}

void vk_rhi::destroy_swapchain(rhi_swapchain* swapchain)
{
    add_delay_delete(swapchain);
}

rhi_fence* vk_rhi::create_fence()
{
    return new vk_fence(true, m_context.get());
}

void vk_rhi::destroy_fence(rhi_fence* fence)
{
    add_delay_delete(fence);
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