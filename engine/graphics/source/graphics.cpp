#include "graphics.hpp"
#include "context.hpp"
#include "graphics_config.hpp"
#include "log.hpp"
#include "math.hpp"
#include "relation.hpp"
#include "scene.hpp"
#include "window.hpp"
#include "window_event.hpp"
#include <fstream>

using namespace ash::math;

namespace ash::graphics
{
graphics::graphics() noexcept : system_base("graphics")
{
}

bool graphics::initialize(const dictionary& config)
{
    m_config.load(config);

    auto& window = system<ash::window::window>();

    context_config desc = {};
    desc.window_handle = window.handle();
    window::window_rect rect = window.rect();
    desc.width = rect.width;
    desc.height = rect.height;
    desc.render_concurrency = m_config.render_concurrency();
    desc.multiple_sampling = m_config.multiple_sampling();
    desc.frame_resource = m_config.frame_resource();

    if (!m_plugin.load(m_config.plugin()) || !m_plugin.initialize(desc))
        return false;

    m_renderer = m_plugin.renderer();
    m_factory = m_plugin.factory();

    adapter_info info[4] = {};
    std::size_t num_adapter = m_renderer->adapter(info, 4);
    for (std::size_t i = 0; i < num_adapter; ++i)
    {
        log::debug("graphics adapter: {}", info[i].description);
    }

    auto& world = system<ecs::world>();
    auto& event = system<core::event>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    world.register_component<visual>();
    world.register_component<main_camera>();
    world.register_component<camera>();
    m_visual_view = world.make_view<visual>();
    m_object_view = world.make_view<visual, scene::transform>();
    m_camera_view = world.make_view<main_camera, camera, scene::transform>();
    // m_tv = world.make_view<scene::transform>();

    event.subscribe<window::event_window_resize>(
        [this](std::uint32_t width, std::uint32_t height) { m_renderer->resize(width, height); });

    m_debug = std::make_unique<graphics_debug>(m_config.frame_resource(), *this, world);
    m_debug->initialize();
    relation.link(m_debug->entity(), scene.root());

    return true;
}

void graphics::render(ecs::entity camera_entity)
{
    auto& world = system<ecs::world>();

    auto& c = world.component<camera>(camera_entity);
    auto& t = world.component<scene::transform>(camera_entity);

    if (c.mask & visual::mask_type::DEBUG)
        m_debug->sync();

    // Update camera data.
    math::float4x4_simd transform_v;
    math::float4x4_simd transform_p;
    math::float4x4_simd transform_vp;

    if (t.sync_count != 0)
    {
        math::float4x4_simd world_simd = math::simd::load(t.world_matrix);
        transform_v = math::matrix_simd::inverse(world_simd);
        math::simd::store(transform_v, c.view);
    }
    else
    {
        transform_v = math::simd::load(c.view);
    }

    transform_p = math::simd::load(c.projection);
    transform_vp = math::matrix_simd::mul(transform_v, transform_p);

    if (t.sync_count != 0)
    {
        math::float4x4 view, projection, view_projection;
        math::simd::store(math::matrix_simd::transpose(transform_v), view);
        math::simd::store(math::matrix_simd::transpose(transform_p), projection);
        math::simd::store(math::matrix_simd::transpose(transform_vp), view_projection);

        c.parameter->set(0, float4{1.0f, 2.0f, 3.0f, 4.0f});
        c.parameter->set(1, float4{5.0f, 6.0f, 7.0f, 8.0f});
        c.parameter->set(2, view, false);
        c.parameter->set(3, projection, false);
        c.parameter->set(4, view_projection, false);
    }

    // Update object data.
    m_object_view->each([&, this](visual& visual, scene::transform& transform) {
        if ((visual.mask & c.mask) == 0)
            return;

        math::float4x4_simd transform_m = math::simd::load(transform.world_matrix);
        math::float4x4_simd transform_mv = math::matrix_simd::mul(transform_m, transform_v);
        math::float4x4_simd transform_mvp = math::matrix_simd::mul(transform_mv, transform_p);

        math::float4x4 model, model_view, model_view_projection;
        math::simd::store(math::matrix_simd::transpose(transform_m), model);
        math::simd::store(math::matrix_simd::transpose(transform_mv), model_view);
        math::simd::store(math::matrix_simd::transpose(transform_mvp), model_view_projection);

        visual.object->set(0, model, false);
        visual.object->set(1, model_view, false);
        visual.object->set(2, model_view_projection, false);
    });

    m_visual_view->each([&, this](visual& visual) {
        if ((visual.mask & c.mask) == 0)
            return;

        for (std::size_t i = 0; i < visual.submesh.size(); ++i)
        {
            m_render_pipelines.insert(visual.submesh[i].pipeline);
            visual.submesh[i].pipeline->add(&visual.submesh[i]);
        }
    });

    // Render.
    auto command = m_renderer->allocate_command();

    if (c.render_target == nullptr)
    {
        for (auto pipeline : m_render_pipelines)
            pipeline->render(
                m_renderer->back_buffer(),
                m_renderer->depth_stencil(),
                command,
                c.parameter.get());
    }
    else
    {
        command->begin_render(c.render_target);
        command->clear_render_target(c.render_target);
        command->clear_depth_stencil(c.depth_stencil);
        for (auto pipeline : m_render_pipelines)
            pipeline->render(c.render_target, c.depth_stencil, command, c.parameter.get());
        command->end_render(c.render_target);
    }

    for (auto pipeline : m_render_pipelines)
        pipeline->clear();
    m_render_pipelines.clear();

    m_renderer->execute(command);
}

void graphics::begin_frame()
{
    m_renderer->begin_frame();
    m_debug->begin_frame();
}

void graphics::end_frame()
{
    m_debug->end_frame();
    m_renderer->end_frame();
}

std::unique_ptr<resource> graphics::make_texture(std::string_view file)
{
    std::ifstream fin(file.data(), std::ios::in | std::ios::binary);
    if (!fin)
    {
        log::error("Can not open texture: {}.", file);
        return nullptr;
    }

    std::vector<uint8_t> dds_data(fin.seekg(0, std::ios::end).tellg());
    fin.seekg(0, std::ios::beg).read((char*)dds_data.data(), dds_data.size());
    fin.close();

    return std::unique_ptr<resource>(m_factory->make_texture(dds_data.data(), dds_data.size()));
}

std::tuple<bool, std::size_t, std::size_t> graphics::make_pipeline(
    std::string_view name,
    pipeline_layout*& layout,
    pipeline*& pipeline)
{
    auto [found, desc, config] = m_config.find_desc<pipeline_desc>(name);
    if (!found)
        return {false, 0, 0};

    // make layout
    pipeline_layout_desc layout_desc = {};
    layout_desc.size = 0;

    for (auto& parameter : config->unit_parameters)
    {
        auto [parameter_found, parameter_desc, _] =
            m_config.find_desc<pipeline_parameter_desc>(parameter);
        if (!parameter_found)
        {
            log::error("unit parameter no found: {}", parameter);
            return {false, 0, 0};
        }
        layout_desc.data[layout_desc.size] = parameter_desc;
        ++layout_desc.size;
    }

    for (auto& parameter : config->pass_parameters)
    {
        auto [parameter_found, parameter_desc, _] =
            m_config.find_desc<pipeline_parameter_desc>(parameter);
        if (!parameter_found)
        {
            log::error("pass parameter no found: {}", parameter);
            return {false, 0, 0};
        }
        layout_desc.data[layout_desc.size] = parameter_desc;
        ++layout_desc.size;
    }

    layout = m_factory->make_pipeline_layout(layout_desc);
    desc.layout = layout;
    pipeline = m_factory->make_pipeline(desc);

    return {true, config->unit_parameters.size(), config->pass_parameters.size()};
}
} // namespace ash::graphics