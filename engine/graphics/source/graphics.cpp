#include "graphics/graphics.hpp"
#include "core/context.hpp"
#include "graphics/camera.hpp"
#include "graphics/graphics_event.hpp"
#include "graphics/graphics_task.hpp"
#include "graphics/rhi.hpp"
#include "graphics/skin_pipeline.hpp"
#include "graphics/sky_pipeline.hpp"
#include "graphics/standard_pipeline.hpp"
#include "scene/scene.hpp"
#include "task/task_manager.hpp"
#include "window/window.hpp"
#include "window/window_event.hpp"
#include <set>

using namespace ash::math;

namespace ash::graphics
{
graphics::graphics() noexcept
    : system_base("graphics"),
      m_back_buffer_index(0),
      m_game_camera(ecs::INVALID_ENTITY),
      m_editor_camera(ecs::INVALID_ENTITY)
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
    rhi::register_pipeline_parameter_layout("ash_sky", sky_pipeline_parameter::layout());

    m_sky_texture = rhi::make_texture_cube(
        "engine/texture/skybox/cloudymorning_left.png",
        "engine/texture/skybox/cloudymorning_right.png",
        "engine/texture/skybox/cloudymorning_top.png",
        "engine/texture/skybox/cloudymorning_bottom.png",
        "engine/texture/skybox/cloudymorning_front.png",
        "engine/texture/skybox/cloudymorning_back.png");
    m_sky_parameter = std::make_unique<sky_pipeline_parameter>();
    m_sky_parameter->texture(m_sky_texture.get());
    m_sky_pipeline = std::make_unique<sky_pipeline>();

    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    world.register_component<mesh_render>();
    world.register_component<skinned_mesh>();
    world.register_component<camera>();

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
    m_debug = nullptr;
}

void graphics::compute(compute_pipeline* pipeline)
{
    auto command = rhi::renderer().allocate_command();
    pipeline->compute(command);
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

void graphics::skin_meshes()
{
    auto& world = system<ecs::world>();

    auto command = rhi::renderer().allocate_command();

    std::set<skin_pipeline*> pipelines;
    world.view<mesh_render, skinned_mesh>().each(
        [&](mesh_render& mesh_render, skinned_mesh& skinned_mesh) {
            auto pipeline = skinned_mesh.pipeline;
            pipelines.insert(pipeline);
            pipeline->add(skinned_mesh);
        });

    for (auto pipeline : pipelines)
    {
        pipeline->skin(command);
        pipeline->clear();
    }

    world.view<mesh_render, skinned_mesh>().each(
        [&](mesh_render& mesh_render, skinned_mesh& skinned_mesh) {
            for (std::size_t i = 0; i < skinned_mesh.skinned_vertex_buffers.size(); ++i)
            {
                if (skinned_mesh.skinned_vertex_buffers[i] != nullptr)
                    mesh_render.vertex_buffers[i] = skinned_mesh.skinned_vertex_buffers[i].get();
            }
        });

    rhi::renderer().execute(command);
}

void graphics::render()
{
    auto& world = system<ecs::world>();

    ecs::entity main_camera = is_editor_mode() ? m_editor_camera : m_game_camera;
    ASH_ASSERT(main_camera != ecs::INVALID_ENTITY);
    world.component<camera>(main_camera).render_target_resolve(rhi::renderer().back_buffer());
    m_render_queue.push(main_camera);

    // Upload debug draw data.
    m_debug->sync();

    // Skin.
    skin_meshes();

    // Update object data.
    world.view<mesh_render, scene::transform>().each(
        [&, this](mesh_render& mesh_render, scene::transform& transform) {
            mesh_render.object_parameter->world_matrix(transform.to_world());
        });

    // Render camera.
    while (!m_render_queue.empty())
    {
        render_camera(m_render_queue.front());
        m_render_queue.pop();
    }
}

void graphics::render_camera(ecs::entity camera_entity)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();

    auto& render_camera = world.component<camera>(camera_entity);
    auto& transform = world.component<scene::transform>(camera_entity);

    // Update camera data.
    math::float4x4_simd world_simd = math::simd::load(transform.to_world());
    math::float4x4_simd transform_v = math::matrix_simd::inverse(world_simd);
    math::float4x4_simd transform_p = math::simd::load(render_camera.projection());
    math::float4x4_simd transform_vp = math::matrix_simd::mul(transform_v, transform_p);

    math::float3 position;
    math::simd::store(world_simd[3], position);
    render_camera.pipeline_parameter()->position(position);

    math::float4x4 view, view_projection;
    math::simd::store(transform_v, view);
    math::simd::store(transform_vp, view_projection);
    render_camera.pipeline_parameter()->view(view);
    render_camera.pipeline_parameter()->projection(render_camera.projection());
    render_camera.pipeline_parameter()->view_projection(view_projection);

    // Frustum culling.
    math::float4_simd x = math::simd::set(
        view_projection[0][0],
        view_projection[1][0],
        view_projection[2][0],
        view_projection[3][0]);
    math::float4_simd y = math::simd::set(
        view_projection[0][1],
        view_projection[1][1],
        view_projection[2][1],
        view_projection[3][1]);
    math::float4_simd z = math::simd::set(
        view_projection[0][2],
        view_projection[1][2],
        view_projection[2][2],
        view_projection[3][2]);
    math::float4_simd w = math::simd::set(
        view_projection[0][3],
        view_projection[1][3],
        view_projection[2][3],
        view_projection[3][3]);

    std::vector<math::float4> planes(6);
    std::vector<math::float4_simd> ps = {
        math::vector_simd::add(w, x),
        math::vector_simd::sub(w, x),
        math::vector_simd::add(w, y),
        math::vector_simd::sub(w, y),
        z,
        math::vector_simd::sub(w, z)};

    for (std::size_t i = 0; i < 6; ++i)
    {
        math::float4_simd length = math::vector_simd::length_vec3_v(ps[i]);
        ps[i] = math::vector_simd::div(ps[i], length);
        math::simd::store(ps[i], planes[i]);
    }
    scene.frustum_culling(planes);

    // Render.
    std::unordered_map<render_pipeline*, render_scene> render_scenes;
    world.view<mesh_render>().each([&, this](mesh_render& mesh_render) {
        if ((mesh_render.render_groups & render_camera.render_groups) == 0)
            return;
        // if ((mesh_render.render_groups & RENDER_GROUP_UI) == 0 && !bounding_box.visible())
        //     return;

        for (std::size_t i = 0; i < mesh_render.materials.size(); ++i)
        {
            auto& render_scene = render_scenes[mesh_render.materials[i].pipeline];
            render_scene.units.emplace_back(render_unit{
                .vertex_buffers = mesh_render.vertex_buffers.data(),
                .index_buffer = mesh_render.index_buffer,
                .index_start = mesh_render.submeshes[i].index_start,
                .index_end = mesh_render.submeshes[i].index_end,
                .vertex_base = mesh_render.submeshes[i].vertex_base,
                .parameters = mesh_render.materials[i].parameters.data(),
                .scissor = mesh_render.materials[i].scissor});
        }
    });

    auto command = rhi::renderer().allocate_command();
    command->clear_render_target(render_camera.render_target(), {0.0f, 0.0f, 0.0f, 1.0f});
    command->clear_depth_stencil(render_camera.depth_stencil_buffer());

    for (auto& [pipeline, render_scene] : render_scenes)
    {
        render_scene.camera_parameter = render_camera.pipeline_parameter()->interface();
        render_scene.light_parameter = m_light_parameter.get();
        render_scene.render_target = render_camera.render_target();
        render_scene.render_target_resolve = render_camera.render_target_resolve();
        render_scene.depth_stencil_buffer = render_camera.depth_stencil_buffer();
        pipeline->render(render_scene, command);
    }

    if (camera_entity != m_editor_camera)
    {
        // Render sky.
        render_scene sky_scene;
        sky_scene.camera_parameter = render_camera.pipeline_parameter()->interface();
        sky_scene.light_parameter = m_light_parameter.get();
        sky_scene.sky_parameter = m_sky_parameter->interface();
        sky_scene.render_target = render_camera.render_target();
        sky_scene.render_target_resolve = render_camera.render_target_resolve();
        sky_scene.depth_stencil_buffer = render_camera.depth_stencil_buffer();
        m_sky_pipeline->render(sky_scene, command);
    }

    rhi::renderer().execute(command);
}

void graphics::present()
{
    rhi::renderer().present();
    m_debug->next_frame();
}
} // namespace ash::graphics