#include "graphics/graphics.hpp"
#include "core/context.hpp"
#include "graphics/graphics_event.hpp"
#include "graphics/graphics_task.hpp"
#include "graphics/rhi.hpp"
#include "graphics/skin_pipeline.hpp"
#include "graphics/standard_pipeline.hpp"
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
      m_render_view(nullptr),
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

    rhi::register_pipeline_parameter_layout("ash_object", object_pipeline_parameter::layout());
    rhi::register_pipeline_parameter_layout("ash_camera", camera_pipeline_parameter::layout());
    rhi::register_pipeline_parameter_layout(
        "standard_material",
        standard_material_pipeline_parameter::layout());

    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    world.register_component<mesh_render>();
    world.register_component<skinned_mesh>();
    world.register_component<camera>();
    m_render_view = world.make_view<mesh_render>();
    m_object_view = world.make_view<mesh_render, scene::transform>();
    m_skinned_mesh_view = world.make_view<mesh_render, skinned_mesh>();

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
    world.destroy_view(m_render_view);
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
    m_skinned_mesh_view->each([&](mesh_render& mesh_render, skinned_mesh& skinned_mesh) {
        auto pipeline = skinned_mesh.pipeline;
        pipelines.insert(pipeline);
        pipeline->add(skinned_mesh);
    });

    for (auto pipeline : pipelines)
    {
        pipeline->skin(command);
        pipeline->clear();
    }

    m_skinned_mesh_view->each([&](mesh_render& mesh_render, skinned_mesh& skinned_mesh) {
        for (std::size_t i = 0; i < skinned_mesh.skinned_vertex_buffers.size(); ++i)
        {
            if (skinned_mesh.skinned_vertex_buffers[i] != nullptr)
                mesh_render.vertex_buffers[i] = skinned_mesh.skinned_vertex_buffers[i].get();
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
    ASH_ASSERT(main_camera != ecs::INVALID_ENTITY);
    world.component<camera>(main_camera).render_target_resolve(rhi::renderer().back_buffer());
    m_render_queue.push(main_camera);

    // Update object data.
    m_object_view->each([&, this](mesh_render& mesh_render, scene::transform& transform) {
        mesh_render.object_parameter->world_matrix(transform.to_world());
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

    if (render_camera.render_groups & RENDER_GROUP_DEBUG)
        m_debug->sync();

    // Update camera data.
    math::float4x4_simd world_simd = math::simd::load(transform.to_world());
    math::float4x4_simd transform_v = math::matrix_simd::inverse(world_simd);
    math::float4x4_simd transform_p = math::simd::load(render_camera.projection());
    math::float4x4_simd transform_vp = math::matrix_simd::mul(transform_v, transform_p);

    math::float4x4 view, view_projection;
    math::simd::store(transform_v, view);
    math::simd::store(transform_vp, view_projection);
    render_camera.pipeline_parameter()->view(view);
    render_camera.pipeline_parameter()->projection(render_camera.projection());
    render_camera.pipeline_parameter()->view_projection(view_projection);

    std::unordered_map<render_pipeline*, render_scene> render_scenes;
    m_render_view->each([&, this](mesh_render& mesh_render) {
        if ((mesh_render.render_groups & render_camera.render_groups) == 0)
            return;

        for (std::size_t i = 0; i < mesh_render.materials.size(); ++i)
        {
            auto& render_scene = render_scenes[mesh_render.materials[i].pipeline];
            render_scene.units.push_back(render_unit{
                .vertex_buffers = mesh_render.vertex_buffers,
                .index_buffer = mesh_render.index_buffer,
                .index_start = mesh_render.submeshes[i].index_start,
                .index_end = mesh_render.submeshes[i].index_end,
                .vertex_base = mesh_render.submeshes[i].vertex_base,
                .parameters = mesh_render.materials[i].parameters,
                .scissor = mesh_render.materials[i].scissor});
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