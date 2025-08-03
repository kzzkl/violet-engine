#include "graphics/render_device.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/resources/texture.hpp"
#include "shader_compiler.hpp"
#include "transient_allocator.hpp"
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

void rhi_deleter::operator()(rhi_raster_pipeline* raster_pipeline)
{
    m_rhi->destroy_raster_pipeline(raster_pipeline);
}

void rhi_deleter::operator()(rhi_compute_pipeline* compute_pipeline)
{
    m_rhi->destroy_compute_pipeline(compute_pipeline);
}

void rhi_deleter::operator()(rhi_parameter* parameter)
{
    m_rhi->destroy_parameter(parameter);
}

void rhi_deleter::operator()(rhi_sampler* sampler)
{
    m_rhi->destroy_sampler(sampler);
}

void rhi_deleter::operator()(rhi_buffer* buffer)
{
    auto* transient_allocator = render_device::instance().m_transient_allocator.get();
    if (transient_allocator != nullptr)
    {
        transient_allocator->cleanup_dependents(buffer);
    }

    m_rhi->destroy_buffer(buffer);
}

void rhi_deleter::operator()(rhi_texture* texture)
{
    auto* transient_allocator = render_device::instance().m_transient_allocator.get();
    if (transient_allocator != nullptr)
    {
        transient_allocator->cleanup_dependents(texture);
    }

    m_rhi->destroy_texture(texture);
}

void rhi_deleter::operator()(rhi_swapchain* swapchain)
{
    auto* transient_allocator = render_device::instance().m_transient_allocator.get();
    if (transient_allocator != nullptr)
    {
        for (std::uint32_t i = 0; i < swapchain->get_texture_count(); ++i)
        {
            transient_allocator->cleanup_dependents(swapchain->get_texture(i));
        }
    }

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

    m_material_manager = std::make_unique<material_manager>();
    m_geometry_manager = std::make_unique<geometry_manager>();

    m_transient_allocator = std::make_unique<transient_allocator>();

    create_buildin_resources();
}

void render_device::reset()
{
    m_transient_allocator = nullptr;

    m_shaders.clear();

    m_material_manager = nullptr;
    m_geometry_manager = nullptr;

    m_buildin_textures.clear();
    m_buildin_samplers.clear();

    m_rhi_deleter = {};
    m_rhi = nullptr;
}

rhi_command* render_device::allocate_command()
{
    return m_rhi->allocate_command();
}

void render_device::execute(rhi_command* command, bool sync)
{
    m_rhi->execute(command, sync);
}

void render_device::begin_frame()
{
    m_rhi->begin_frame();
    m_transient_allocator->tick();
}

void render_device::end_frame()
{
    m_rhi->end_frame();
}

std::uint32_t render_device::get_frame_count() const noexcept
{
    return m_rhi->get_frame_count();
}

std::uint32_t render_device::get_frame_resource_count() const noexcept
{
    return m_rhi->get_frame_resource_count();
}

std::uint32_t render_device::get_frame_resource_index() const noexcept
{
    return m_rhi->get_frame_resource_index();
}

rhi_parameter* render_device::get_bindless_parameter() const noexcept
{
    return m_rhi->get_bindless_parameter();
}

rhi_ptr<rhi_render_pass> render_device::create_render_pass(const rhi_render_pass_desc& desc)
{
    return {m_rhi->create_render_pass(desc), m_rhi_deleter};
}

rhi_ptr<rhi_raster_pipeline> render_device::create_pipeline(const rhi_raster_pipeline_desc& desc)
{
    return {m_rhi->create_raster_pipeline(desc), m_rhi_deleter};
}

rhi_ptr<rhi_compute_pipeline> render_device::create_pipeline(const rhi_compute_pipeline_desc& desc)
{
    return {m_rhi->create_compute_pipeline(desc), m_rhi_deleter};
}

rhi_ptr<rhi_parameter> render_device::create_parameter(const rhi_parameter_desc& desc)
{
    return {m_rhi->create_parameter(desc), m_rhi_deleter};
}

rhi_ptr<rhi_sampler> render_device::create_sampler(const rhi_sampler_desc& desc)
{
    return {m_rhi->create_sampler(desc), m_rhi_deleter};
}

rhi_ptr<rhi_buffer> render_device::create_buffer(const rhi_buffer_desc& desc)
{
    return {m_rhi->create_buffer(desc), m_rhi_deleter};
}

rhi_ptr<rhi_texture> render_device::create_texture(const rhi_texture_desc& desc)
{
    return {m_rhi->create_texture(desc), m_rhi_deleter};
}

rhi_ptr<rhi_swapchain> render_device::create_swapchain(const rhi_swapchain_desc& desc)
{
    return {m_rhi->create_swapchain(desc), m_rhi_deleter};
}

rhi_ptr<rhi_fence> render_device::create_fence()
{
    return {m_rhi->create_fence(), m_rhi_deleter};
}

rhi_parameter* render_device::allocate_parameter(const rhi_parameter_desc& desc)
{
    return m_transient_allocator->allocate_parameter(desc);
}

rhi_texture* render_device::allocate_texture(const rhi_texture_desc& desc)
{
    return m_transient_allocator->allocate_texture(desc);
}

rhi_buffer* render_device::allocate_buffer(const rhi_buffer_desc& desc)
{
    return m_transient_allocator->allocate_buffer(desc);
}

rhi_render_pass* render_device::get_render_pass(const rhi_render_pass_desc& desc)
{
    return m_transient_allocator->get_render_pass(desc);
}

rhi_raster_pipeline* render_device::get_pipeline(const rhi_raster_pipeline_desc& desc)
{
    return m_transient_allocator->get_pipeline(desc);
}

rhi_compute_pipeline* render_device::get_pipeline(const rhi_compute_pipeline_desc& desc)
{
    return m_transient_allocator->get_pipeline(desc);
}

rhi_sampler* render_device::get_sampler(const rhi_sampler_desc& desc)
{
    return m_transient_allocator->get_sampler(desc);
}

void render_device::create_buildin_resources()
{
    // Create empty texture.
    texture_data::mipmap empty_mipmap_data;
    empty_mipmap_data.extent.width = 1;
    empty_mipmap_data.extent.height = 1;
    empty_mipmap_data.pixels.resize(4);
    *reinterpret_cast<std::uint32_t*>(empty_mipmap_data.pixels.data()) = 0xFFFFFFFF;

    texture_data empty_texture_data;
    empty_texture_data.format = RHI_FORMAT_R8G8B8A8_UNORM;
    empty_texture_data.mipmaps.push_back(empty_mipmap_data);

    auto empty_texture = std::make_unique<texture_2d>(empty_texture_data);
    // Ensure that the SRV bindless index of the empty_texture is 0.
    (void)empty_texture->get_srv();
    assert(empty_texture->get_srv()->get_bindless() == 0);

    m_buildin_textures.push_back(std::move(empty_texture));

    // Create buildin samplers.

    // Bindless index 0: point repeat sampler.
    m_buildin_samplers.push_back(create_sampler({
        .mag_filter = RHI_FILTER_POINT,
        .min_filter = RHI_FILTER_POINT,
        .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
        .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
        .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
        .min_level = 0.0f,
        .max_level = -1.0f,
    }));
    assert(m_buildin_samplers[0]->get_bindless() == 0);

    // Bindless index 1: point mirrored repeat sampler.
    m_buildin_samplers.push_back(create_sampler({
        .mag_filter = RHI_FILTER_POINT,
        .min_filter = RHI_FILTER_POINT,
        .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        .min_level = 0.0f,
        .max_level = -1.0f,
    }));
    assert(m_buildin_samplers[1]->get_bindless() == 1);

    // Bindless index 2: point clamp sampler.
    m_buildin_samplers.push_back(create_sampler({
        .mag_filter = RHI_FILTER_POINT,
        .min_filter = RHI_FILTER_POINT,
        .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .min_level = 0.0f,
        .max_level = -1.0f,
    }));
    assert(m_buildin_samplers[2]->get_bindless() == 2);

    // Bindless index 3: linear repeat sampler.
    m_buildin_samplers.push_back(create_sampler({
        .mag_filter = RHI_FILTER_LINEAR,
        .min_filter = RHI_FILTER_LINEAR,
        .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
        .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
        .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
        .min_level = 0.0f,
        .max_level = -1.0f,
    }));
    assert(m_buildin_samplers[3]->get_bindless() == 3);

    // Bindless index 4: linear mirrored repeat sampler.
    m_buildin_samplers.push_back(create_sampler({
        .mag_filter = RHI_FILTER_LINEAR,
        .min_filter = RHI_FILTER_LINEAR,
        .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        .min_level = 0.0f,
        .max_level = -1.0f,
    }));
    assert(m_buildin_samplers[4]->get_bindless() == 4);

    // Bindless index 5: linear clamp sampler.
    m_buildin_samplers.push_back(create_sampler({
        .mag_filter = RHI_FILTER_LINEAR,
        .min_filter = RHI_FILTER_LINEAR,
        .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .min_level = 0.0f,
        .max_level = -1.0f,
    }));
    assert(m_buildin_samplers[5]->get_bindless() == 5);
}

std::vector<std::uint8_t> render_device::compile_shader(
    std::string_view path,
    std::span<const wchar_t*> arguments)
{
    std::ifstream fin(std::string(path), std::ios::binary);
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