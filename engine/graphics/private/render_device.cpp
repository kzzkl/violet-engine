#include "graphics/render_device.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/tools/ibl_tool.hpp"
#include "graphics/tools/texture_loader.hpp"
#include "shader_compiler.hpp"
#include <fstream>

namespace violet
{
rhi_deleter::rhi_deleter()
    : m_rhi(nullptr)
{
}

rhi_deleter::rhi_deleter(rhi* rhi)
    : m_rhi(rhi)
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

render_device::render_device() = default;

render_device::~render_device() {}

render_device& render_device::instance()
{
    static render_device instance;
    return instance;
}

void render_device::initialize(rhi* rhi)
{
    m_rhi = rhi;
    m_rhi_deleter = rhi_deleter(rhi);

    m_shader_compiler = std::make_unique<shader_compiler>();
    m_fence = create_fence();

    m_material_manager = std::make_unique<material_manager>();
    m_geometry_manager = std::make_unique<geometry_manager>();

    create_buildin_resources();
}

void render_device::reset()
{
    m_shaders.clear();
    m_fence = nullptr;

    m_material_manager = nullptr;
    m_geometry_manager = nullptr;

    m_buildin_resources = {};

    m_rhi_deleter = {};
    m_rhi = nullptr;
}

rhi_command* render_device::allocate_command()
{
    return m_rhi->allocate_command();
}

void render_device::execute(rhi_command* command)
{
    m_rhi->execute(command);
}

void render_device::execute_sync(rhi_command* command)
{
    ++m_fence_value;

    command->signal(m_fence.get(), m_fence_value);
    m_rhi->execute(command);
    m_fence->wait(m_fence_value);
}

void render_device::begin_frame()
{
    m_rhi->begin_frame();
}

void render_device::end_frame()
{
    m_rhi->end_frame();
}

std::size_t render_device::get_frame_count() const noexcept
{
    return m_rhi->get_frame_count();
}

std::size_t render_device::get_frame_resource_count() const noexcept
{
    return m_rhi->get_frame_resource_count();
}

std::size_t render_device::get_frame_resource_index() const noexcept
{
    return m_rhi->get_frame_resource_index();
}

rhi_parameter* render_device::get_bindless_parameter() const noexcept
{
    return m_rhi->get_bindless_parameter();
}

rhi_ptr<rhi_render_pass> render_device::create_render_pass(const rhi_render_pass_desc& desc)
{
    return rhi_ptr<rhi_render_pass>(m_rhi->create_render_pass(desc), m_rhi_deleter);
}

rhi_ptr<rhi_render_pipeline> render_device::create_pipeline(const rhi_render_pipeline_desc& desc)
{
    return rhi_ptr<rhi_render_pipeline>(m_rhi->create_render_pipeline(desc), m_rhi_deleter);
}

rhi_ptr<rhi_compute_pipeline> render_device::create_pipeline(const rhi_compute_pipeline_desc& desc)
{
    return rhi_ptr<rhi_compute_pipeline>(m_rhi->create_compute_pipeline(desc), m_rhi_deleter);
}

rhi_ptr<rhi_parameter> render_device::create_parameter(const rhi_parameter_desc& desc)
{
    return rhi_ptr<rhi_parameter>(m_rhi->create_parameter(desc), m_rhi_deleter);
}

rhi_ptr<rhi_framebuffer> render_device::create_framebuffer(const rhi_framebuffer_desc& desc)
{
    return rhi_ptr<rhi_framebuffer>(m_rhi->create_framebuffer(desc), m_rhi_deleter);
}

rhi_ptr<rhi_buffer> render_device::create_buffer(const rhi_buffer_desc& desc)
{
    return rhi_ptr<rhi_buffer>(m_rhi->create_buffer(desc), m_rhi_deleter);
}

rhi_ptr<rhi_sampler> render_device::create_sampler(const rhi_sampler_desc& desc)
{
    return rhi_ptr<rhi_sampler>(m_rhi->create_sampler(desc), m_rhi_deleter);
}

rhi_ptr<rhi_texture> render_device::create_texture(const rhi_texture_desc& desc)
{
    return rhi_ptr<rhi_texture>(m_rhi->create_texture(desc), m_rhi_deleter);
}

rhi_ptr<rhi_texture> render_device::create_texture(const rhi_texture_view_desc& desc)
{
    return rhi_ptr<rhi_texture>(m_rhi->create_texture(desc), m_rhi_deleter);
}

rhi_ptr<rhi_swapchain> render_device::create_swapchain(const rhi_swapchain_desc& desc)
{
    return rhi_ptr<rhi_swapchain>(m_rhi->create_swapchain(desc), m_rhi_deleter);
}

rhi_ptr<rhi_fence> render_device::create_fence()
{
    return rhi_ptr<rhi_fence>(m_rhi->create_fence(), m_rhi_deleter);
}

void render_device::create_buildin_resources()
{
    m_buildin_resources.material_buffer = m_material_manager->get_material_buffer();

    // Create empty texture.
    texture_loader::mipmap_data empty_mipmap_data;
    empty_mipmap_data.extent.width = 1;
    empty_mipmap_data.extent.height = 1;
    empty_mipmap_data.pixels.resize(4);
    *reinterpret_cast<std::uint32_t*>(empty_mipmap_data.pixels.data()) = 0xFFFFFFFF;

    texture_loader::texture_data empty_texture_data;
    empty_texture_data.format = RHI_FORMAT_R8G8B8A8_UNORM;
    empty_texture_data.mipmaps.push_back(empty_mipmap_data);

    m_buildin_resources.empty_texture = texture_loader::load(empty_texture_data);

    // Create brdf lut texture.
    m_buildin_resources.brdf_lut = create_texture({
        .extent = {512, 512},
        .format = RHI_FORMAT_R32G32_FLOAT,
        .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_RENDER_TARGET,
        .level_count = 1,
        .layer_count = 1,
        .samples = RHI_SAMPLE_COUNT_1,
    });

    ibl_tool::generate_brdf_lut(m_buildin_resources.brdf_lut.get());

    m_buildin_resources.point_repeat_sampler = create_sampler({
        .mag_filter = RHI_FILTER_POINT,
        .min_filter = RHI_FILTER_POINT,
        .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
        .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
        .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
    });

    m_buildin_resources.point_clamp_sampler = create_sampler({
        .mag_filter = RHI_FILTER_POINT,
        .min_filter = RHI_FILTER_POINT,
        .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    });

    m_buildin_resources.linear_repeat_sampler = create_sampler({
        .mag_filter = RHI_FILTER_LINEAR,
        .min_filter = RHI_FILTER_LINEAR,
        .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
        .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
        .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
        .min_level = 0.0,
        .max_level = 10.0,
    });

    m_buildin_resources.linear_clamp_sampler = create_sampler({
        .mag_filter = RHI_FILTER_LINEAR,
        .min_filter = RHI_FILTER_LINEAR,
        .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .min_level = 0.0,
        .max_level = 10.0,
    });
}

std::vector<std::uint8_t> render_device::compile_shader(
    std::string_view path,
    std::span<const wchar_t*> arguments)
{
    std::ifstream fin(path.data(), std::ios::binary);
    if (!fin.is_open())
    {
        throw std::runtime_error("Failed to open file!");
    }

    fin.seekg(0, std::ios::end);

    std::vector<char> source(fin.tellg());
    fin.seekg(0);
    fin.read(source.data(), static_cast<std::streamsize>(source.size()));
    fin.close();

    return m_shader_compiler->compile(source, arguments);
}
} // namespace violet