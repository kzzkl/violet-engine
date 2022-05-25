#include "graphics/graphics.hpp"
#include "core/context.hpp"
#include "graphics/graphics_config.hpp"
#include "graphics/skin_pipeline.hpp"
#include "log.hpp"
#include "math/math.hpp"
#include "scene/transform.hpp"
#include "window/window.hpp"
#include "window/window_event.hpp"
#include <fstream>
#include <set>

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

    renderer_desc desc = {};
    desc.window_handle = window.handle();
    window::window_extent extent = window.extent();
    desc.width = extent.width;
    desc.height = extent.height;
    desc.render_concurrency = m_config.render_concurrency();
    desc.frame_resource = m_config.frame_resource();

    if (!m_plugin.load(m_config.plugin()))
        return false;

    m_renderer.reset(m_plugin.factory().make_renderer(desc));

    pipeline_parameter_layout_info ash_object;
    ash_object.parameters = {
        {pipeline_parameter_type::FLOAT4x4, 1}, // transform_m
        {pipeline_parameter_type::FLOAT4x4, 1}, // transform_mv
        {pipeline_parameter_type::FLOAT4x4, 1}  // transform_mvp
    };
    make_pipeline_parameter_layout("ash_object", ash_object);

    pipeline_parameter_layout_info ash_pass;
    ash_pass.parameters = {
        {pipeline_parameter_type::FLOAT4,   1}, // camera_position
        {pipeline_parameter_type::FLOAT4,   1}, // camera_direction
        {pipeline_parameter_type::FLOAT4x4, 1}, // transform_v
        {pipeline_parameter_type::FLOAT4x4, 1}, // transform_p
        {pipeline_parameter_type::FLOAT4x4, 1}  // transform_vp
    };
    make_pipeline_parameter_layout("ash_pass", ash_pass);

    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    world.register_component<visual>();
    world.register_component<skinned_mesh>();
    world.register_component<camera>();
    world.register_component<main_camera>();
    m_visual_view = world.make_view<visual>();
    m_object_view = world.make_view<visual, scene::transform>();
    m_skinned_mesh_view = world.make_view<visual, skinned_mesh>();
    // m_tv = world.make_view<scene::transform>();

    event.subscribe<window::event_window_resize>(
        "graphics",
        [this](std::uint32_t width, std::uint32_t height) { m_renderer->resize(width, height); });

    m_debug = std::make_unique<graphics_debug>(m_config.frame_resource());
    m_debug->initialize(*this);

    return true;
}

void graphics::shutdown()
{
    auto& world = system<ecs::world>();
    world.destroy_view(m_visual_view);
    world.destroy_view(m_object_view);
    world.destroy_view(m_skinned_mesh_view);

    m_debug = nullptr;
    m_parameter_layouts.clear();

    m_renderer = nullptr;
    m_plugin.unload();
}

void graphics::skin_meshes()
{
    auto command = m_renderer->allocate_command();

    std::set<skin_pipeline*> pipelines;
    m_skinned_mesh_view->each([&](visual& visual, skinned_mesh& skinned_mesh) {
        auto pipeline = skinned_mesh.pipeline;
        pipelines.insert(pipeline);
        pipeline->add(skinned_mesh);
    });

    for (auto pipeline : pipelines)
    {
        pipeline->skin(command);
        pipeline->clear();
    }

    m_skinned_mesh_view->each([&](visual& visual, skinned_mesh& skinned_mesh) {
        for (std::size_t i = 0; i < skinned_mesh.skinned_vertex_buffers.size(); ++i)
        {
            if (skinned_mesh.skinned_vertex_buffers[i] != nullptr)
                visual.vertex_buffers[i] = skinned_mesh.skinned_vertex_buffers[i].get();
        }
    });

    m_renderer->execute(command);
}

void graphics::render(ecs::entity camera_entity)
{
    auto& world = system<ecs::world>();

    auto& c = world.component<camera>(camera_entity);
    auto& t = world.component<scene::transform>(camera_entity);

    if (c.mask & VISUAL_GROUP_DEBUG)
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
        math::simd::store(transform_v, view);
        math::simd::store(transform_p, projection);
        math::simd::store(transform_vp, view_projection);

        c.parameter->set(0, float4{1.0f, 2.0f, 3.0f, 4.0f});
        c.parameter->set(1, float4{5.0f, 6.0f, 7.0f, 8.0f});
        c.parameter->set(2, view);
        c.parameter->set(3, projection);
        c.parameter->set(4, view_projection);
    }

    // Update object data.
    m_object_view->each([&, this](visual& visual, scene::transform& transform) {
        if ((visual.groups & c.mask) == 0)
            return;

        math::float4x4_simd transform_m = math::simd::load(transform.world_matrix);
        math::float4x4_simd transform_mv = math::matrix_simd::mul(transform_m, transform_v);
        math::float4x4_simd transform_mvp = math::matrix_simd::mul(transform_mv, transform_p);

        math::float4x4 model, model_view, model_view_projection;
        math::simd::store(transform_m, model);
        math::simd::store(transform_mv, model_view);
        math::simd::store(transform_mvp, model_view_projection);

        visual.object->set(0, model);
        visual.object->set(1, model_view);
        visual.object->set(2, model_view_projection);
    });

    std::set<render_pipeline*> pipelines;
    m_visual_view->each([&, this](visual& visual) {
        if ((visual.groups & c.mask) == 0)
            return;

        for (std::size_t i = 0; i < visual.materials.size(); ++i)
        {
            auto pipeline = visual.materials[i].pipeline;
            pipelines.insert(pipeline);
            pipeline->add(visual, i);
        }
    });

    // Render.
    auto command = m_renderer->allocate_command();
    command->clear_render_target(c.render_target, {0.0f, 0.0f, 0.0f, 1.0f});
    command->clear_depth_stencil(c.depth_stencil_buffer);

    if (world.has_component<main_camera>(camera_entity))
        c.render_target_resolve = m_renderer->back_buffer();

    for (auto pipeline : pipelines)
    {
        pipeline->render(c, command);
        pipeline->clear();
    }

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

std::unique_ptr<render_pass_interface> graphics::make_render_pass(render_pass_info& info)
{
    auto& factory = m_plugin.factory();
    for (auto& subpass : info.subpasses)
    {
        for (std::size_t i = 0; i < subpass.parameters.size(); ++i)
        {
            ASH_ASSERT(
                m_parameter_layouts.find(subpass.parameters[i]) != m_parameter_layouts.end());
            subpass.parameter_interfaces.push_back(
                m_parameter_layouts[subpass.parameters[i]].get());
        }
    }
    return std::unique_ptr<render_pass_interface>(factory.make_render_pass(info.convert()));
}

std::unique_ptr<compute_pipeline_interface> graphics::make_compute_pipeline(
    compute_pipeline_info& info)
{
    auto& factory = m_plugin.factory();
    for (std::size_t i = 0; i < info.parameters.size(); ++i)
        info.parameter_interfaces.push_back(m_parameter_layouts[info.parameters[i]].get());

    return std::unique_ptr<compute_pipeline_interface>(
        factory.make_compute_pipeline(info.convert()));
}

void graphics::make_pipeline_parameter_layout(
    std::string_view name,
    pipeline_parameter_layout_info& info)
{
    auto& factory = m_plugin.factory();
    m_parameter_layouts[name.data()].reset(factory.make_pipeline_parameter_layout(info.convert()));
}

std::unique_ptr<pipeline_parameter> graphics::make_pipeline_parameter(std::string_view name)
{
    auto layout = m_parameter_layouts[name.data()].get();
    ASH_ASSERT(layout);
    auto& factory = m_plugin.factory();
    return std::make_unique<pipeline_parameter>(factory.make_pipeline_parameter(layout));
}

std::unique_ptr<resource> graphics::make_texture(std::string_view file)
{
    auto& factory = m_plugin.factory();
    return std::unique_ptr<resource>(factory.make_texture(file.data()));
}
} // namespace ash::graphics