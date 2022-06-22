#include "graphics/graphics.hpp"
#include "core/context.hpp"
#include "graphics/graphics_config.hpp"
#include "graphics/graphics_event.hpp"
#include "graphics/graphics_task.hpp"
#include "graphics/skin_pipeline.hpp"
#include "scene/transform.hpp"
#include "task/task_manager.hpp"
#include "window/window.hpp"
#include "window/window_event.hpp"
#include <fstream>
#include <set>

using namespace ash::math;

namespace ash::graphics
{
graphics::graphics() noexcept
    : system_base("graphics"),
      m_back_buffer_index(0),
      m_game_camera(ecs::INVALID_ENTITY),
      m_editor_camera(ecs::INVALID_ENTITY),
      m_visual_view(nullptr),
      m_object_view(nullptr),
      m_skinned_mesh_view(nullptr)
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

    pipeline_parameter_layout_info ash_light;
    ash_light.parameters = {
        {pipeline_parameter_type::FLOAT3, 1}, // ambient_light
        {pipeline_parameter_type::FLOAT3, 1}
    };
    make_pipeline_parameter_layout("ash_light", ash_light);

    m_light_parameter = make_pipeline_parameter("ash_light");
    m_light_parameter->set(0, math::float3{1.0f, 0.0f, 0.0f});

    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    world.register_component<visual>();
    world.register_component<skinned_mesh>();
    world.register_component<camera>();
    m_visual_view = world.make_view<visual>();
    m_object_view = world.make_view<visual, scene::transform>();
    m_skinned_mesh_view = world.make_view<visual, skinned_mesh>();

    event.register_event<event_render_extent_change>();
    event.subscribe<window::event_window_resize>(
        "graphics",
        [&, this](std::uint32_t width, std::uint32_t height) {
            m_renderer->resize(width, height);

            // The event_render_extent_change event is emitted by the editor module in editor mode.
            if (!is_editor_mode())
                event.publish<event_render_extent_change>(width, height);
        });

    m_debug = std::make_unique<graphics_debug>(m_config.frame_resource());
    m_debug->initialize(*this);

    auto& task = system<task::task_manager>();
    auto render_task = task.schedule(TASK_GRAPHICS_RENDER, [this]() {
        render();
        present();
    });
    render_task->add_dependency(*task.find(task::TASK_GAME_LOGIC_END));

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

void graphics::compute(compute_pipeline* pipeline)
{
    auto command = m_renderer->allocate_command();
    pipeline->compute(command);
    m_renderer->execute(command);
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

void graphics::render(ecs::entity target_camera)
{
    m_render_queue.push(target_camera);
}

resource_extent graphics::render_extent() const noexcept
{
    if (is_editor_mode())
    {
        auto& world = system<ecs::world>();
        auto render_target = world.component<camera>(m_scene_camera).render_target();
        if (render_target != nullptr)
            return render_target->extent();
        else
            return m_renderer->back_buffer()->extent();
    }
    else
    {
        return m_renderer->back_buffer()->extent();
    }
}

std::unique_ptr<render_pipeline_interface> graphics::make_render_pipeline(
    render_pipeline_info& info)
{
    auto& factory = m_plugin.factory();
    for (auto& pass : info.passes)
    {
        for (std::size_t i = 0; i < pass.parameters.size(); ++i)
        {
            ASH_ASSERT(m_parameter_layouts.find(pass.parameters[i]) != m_parameter_layouts.end());
            pass.parameter_interfaces.push_back(m_parameter_layouts[pass.parameters[i]].get());
        }
    }
    return std::unique_ptr<render_pipeline_interface>(factory.make_render_pipeline(info.convert()));
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

void graphics::render()
{
    auto& world = system<ecs::world>();

    ecs::entity main_camera = is_editor_mode() ? m_editor_camera : m_game_camera;
    world.component<camera>(main_camera).render_target_resolve(m_renderer->back_buffer());
    m_render_queue.push(main_camera);

    // Update object data.
    m_object_view->each([&, this](visual& visual, scene::transform& transform) {
        visual.object->set(0, transform.world_matrix);
    });

    // Render camera.
    while (!m_render_queue.empty())
    {
        render_camera(m_render_queue.front());
        m_render_queue.pop();
    }
}

void graphics::present()
{
    m_renderer->present();
    m_debug->next_frame();
}

void graphics::render_camera(ecs::entity camera_entity)
{
    auto& world = system<ecs::world>();

    auto& render_camera = world.component<camera>(camera_entity);
    auto& transform = world.component<scene::transform>(camera_entity);

    if (render_camera.mask & VISUAL_GROUP_DEBUG)
        m_debug->sync();

    // Update camera data.
    math::float4x4_simd world_simd = math::simd::load(transform.world_matrix);
    math::float4x4_simd transform_v = math::matrix_simd::inverse(world_simd);
    math::float4x4_simd transform_p = math::simd::load(render_camera.projection());
    math::float4x4_simd transform_vp = math::matrix_simd::mul(transform_v, transform_p);

    math::float4x4 view, projection, view_projection;
    math::simd::store(transform_v, view);
    math::simd::store(transform_p, projection);
    math::simd::store(transform_vp, view_projection);

    auto parameter = render_camera.parameter();
    parameter->set(0, float4{1.0f, 2.0f, 3.0f, 4.0f});
    parameter->set(1, float4{5.0f, 6.0f, 7.0f, 8.0f});
    parameter->set(2, view);
    parameter->set(3, projection);
    parameter->set(4, view_projection);

    std::unordered_map<render_pipeline*, render_scene> render_scenes;
    m_visual_view->each([&, this](visual& visual) {
        if ((visual.groups & render_camera.mask) == 0)
            return;

        for (std::size_t i = 0; i < visual.materials.size(); ++i)
        {
            auto& render_scene = render_scenes[visual.materials[i].pipeline];
            render_scene.units.push_back(render_unit{
                .vertex_buffers = visual.vertex_buffers,
                .index_buffer = visual.index_buffer,
                .index_start = visual.submeshes[i].index_start,
                .index_end = visual.submeshes[i].index_end,
                .vertex_base = visual.submeshes[i].vertex_base,
                .parameters = visual.materials[i].parameters,
                .scissor = visual.materials[i].scissor});
        }
    });

    // Render.
    auto command = m_renderer->allocate_command();
    command->clear_render_target(render_camera.render_target(), {0.0f, 0.0f, 0.0f, 1.0f});
    command->clear_depth_stencil(render_camera.depth_stencil_buffer());

    for (auto& [pipeline, render_scene] : render_scenes)
    {
        render_scene.light_parameter = m_light_parameter.get();
        pipeline->render(render_camera, render_scene, command);
    }

    m_renderer->execute(command);
}
} // namespace ash::graphics