#include "graphics/graphics.hpp"
#include "core/context.hpp"
#include "graphics/graphics_event.hpp"
#include "graphics/graphics_task.hpp"
#include "graphics/rhi.hpp"
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
    auto& window = system<ash::window::window>();

    rhi_info info = {};
    info.window_handle = window.handle();
    window::window_extent extent = window.extent();
    info.width = extent.width;
    info.height = extent.height;
    info.render_concurrency = config["render_concurrency"];
    info.frame_resource = config["frame_resource"];
    rhi::initialize(config["plugin"], info);

    std::vector<pipeline_parameter_pair> ash_object = {
        {pipeline_parameter_type::FLOAT4x4, 1}, // transform_m
    };
    rhi::register_pipeline_parameter_layout("ash_object", ash_object);

    std::vector<pipeline_parameter_pair> ash_pass = {
        {pipeline_parameter_type::FLOAT4,   1}, // camera_position
        {pipeline_parameter_type::FLOAT4,   1}, // camera_direction
        {pipeline_parameter_type::FLOAT4x4, 1}, // transform_v
        {pipeline_parameter_type::FLOAT4x4, 1}, // transform_p
        {pipeline_parameter_type::FLOAT4x4, 1}  // transform_vp
    };
    rhi::register_pipeline_parameter_layout("ash_pass", ash_pass);

    std::vector<pipeline_parameter_pair> ash_light = {
        {pipeline_parameter_type::FLOAT3, 1}, // ambient_light
        {pipeline_parameter_type::FLOAT3, 1}
    };
    rhi::register_pipeline_parameter_layout("ash_light", ash_light);

    m_light_parameter = rhi::make_pipeline_parameter("ash_light");
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
            rhi::renderer().resize(width, height);

            // The event_render_extent_change event is emitted by the editor module in editor mode.
            if (!is_editor_mode())
                event.publish<event_render_extent_change>(width, height);
        });

    m_debug = std::make_unique<graphics_debug>(config["frame_resource"]);
    m_debug->initialize();

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
}

void graphics::compute(compute_pipeline* pipeline)
{
    auto command = rhi::renderer().allocate_command();
    pipeline->compute(command);
    rhi::renderer().execute(command);
}

void graphics::skin_meshes()
{
    auto command = rhi::renderer().allocate_command();

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

    rhi::renderer().execute(command);
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
            return rhi::renderer().back_buffer()->extent();
    }
    else
    {
        return rhi::renderer().back_buffer()->extent();
    }
}

void graphics::render()
{
    auto& world = system<ecs::world>();

    ecs::entity main_camera = is_editor_mode() ? m_editor_camera : m_game_camera;
    world.component<camera>(main_camera).render_target_resolve(rhi::renderer().back_buffer());
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
    rhi::renderer().present();
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
    auto command = rhi::renderer().allocate_command();
    command->clear_render_target(render_camera.render_target(), {0.0f, 0.0f, 0.0f, 1.0f});
    command->clear_depth_stencil(render_camera.depth_stencil_buffer());

    for (auto& [pipeline, render_scene] : render_scenes)
    {
        render_scene.light_parameter = m_light_parameter.get();
        pipeline->render(render_camera, render_scene, command);
    }

    rhi::renderer().execute(command);
}
} // namespace ash::graphics