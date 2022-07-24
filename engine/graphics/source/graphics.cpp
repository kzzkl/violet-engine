#include "graphics/graphics.hpp"
#include "core/context.hpp"
#include "graphics/blinn_phong_pipeline.hpp"
#include "graphics/camera.hpp"
#include "graphics/compute_pipeline.hpp"
#include "graphics/graphics_event.hpp"
#include "graphics/graphics_task.hpp"
#include "graphics/light.hpp"
#include "graphics/rhi.hpp"
#include "graphics/shadow_pipeline.hpp"
#include "graphics/skinning_pipeline.hpp"
#include "graphics/sky_pipeline.hpp"
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
      m_editor_camera(ecs::INVALID_ENTITY),
      m_shadow_parameter_counter(0)
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
    rhi::register_pipeline_parameter_layout("ash_light", light_pipeline_parameter::layout());
    rhi::register_pipeline_parameter_layout("ash_sky", sky_pipeline_parameter::layout());
    rhi::register_pipeline_parameter_layout("ash_shadow", shadow_map_pipeline_parameter::layout());
    rhi::register_pipeline_parameter_layout(
        "ash_blinn_phong_material",
        blinn_phong_material_pipeline_parameter::layout());

    m_light_parameter = std::make_unique<light_pipeline_parameter>();

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

    m_shadow_pipeline = std::make_unique<shadow_pipeline>();

    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    world.register_component<mesh_render>();
    world.register_component<skinned_mesh>();
    world.register_component<camera>();
    world.register_component<directional_light>();

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

void graphics::skinning()
{
    auto& world = system<ecs::world>();

    auto command = rhi::renderer().allocate_command();

    std::set<skinning_pipeline*> pipelines;
    world.view<mesh_render, skinned_mesh>().each(
        [&](mesh_render& mesh_render, skinned_mesh& skinned_mesh) {
            skinned_mesh.pipeline->add(skinned_mesh);
            pipelines.insert(skinned_mesh.pipeline);
        });

    for (auto pipeline : pipelines)
    {
        pipeline->skinning(command);
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
    auto& scene = system<scene::scene>();

    // Update dynamic AABB.
    scene.update_bounding_box();

    ecs::entity main_camera = is_editor_mode() ? m_editor_camera : m_game_camera;
    ASH_ASSERT(main_camera != ecs::INVALID_ENTITY);
    world.component<camera>(main_camera).render_target_resolve(rhi::renderer().back_buffer());
    m_render_queue.push(main_camera);

    // Upload debug draw data.
    m_debug->sync();

    // Skinning.
    skinning();

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

    m_shadow_parameter_counter = 0;
}

void graphics::render_camera(ecs::entity camera_entity)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();

    auto& render_camera = world.component<camera>(camera_entity);
    auto& transform = world.component<scene::transform>(camera_entity);

    // Update camera data.
    auto& to_world = transform.to_world();
    math::float4x4_simd transform_v =
        math::matrix_simd::inverse_transform(math::simd::load(to_world));
    math::float4x4_simd transform_p = math::simd::load(render_camera.projection());
    math::float4x4_simd transform_vp = math::matrix_simd::mul(transform_v, transform_p);

    render_camera.pipeline_parameter()->position(
        math::float3{to_world[3][0], to_world[3][1], to_world[3][2]});
    render_camera.pipeline_parameter()->direction(
        math::float3{to_world[2][0], to_world[2][1], to_world[2][2]});

    math::float4x4 view, view_projection;
    math::simd::store(transform_v, view);
    math::simd::store(transform_vp, view_projection);
    render_camera.pipeline_parameter()->view(view);
    render_camera.pipeline_parameter()->projection(render_camera.projection());
    render_camera.pipeline_parameter()->view_projection(view_projection);

    // Render.
    auto command = rhi::renderer().allocate_command();

    // Update light data and render shadow map.
    auto frustum_vertices = math::utility::frustum_vertices_vec4(view_projection);
    world.view<directional_light, scene::transform>().each(
        [this,
         &frustum_vertices,
         command](ecs::entity entity, directional_light& light, scene::transform& transform) {
            auto& to_world = transform.to_world();
            m_light_parameter->directional_light(
                0,
                light.color(),
                math::float3{to_world[2][0], to_world[2][1], to_world[2][2]});

            render_shadow_map(entity, frustum_vertices, command);
        });
    m_light_parameter->directional_light_count(1);

    // Frustum culling.
    scene.frustum_culling(math::utility::frustum_planes(view_projection));

    // Draw object.
    std::set<render_pipeline*> render_pipelines;
    world.view<mesh_render>().each([&, this](ecs::entity entity, mesh_render& mesh_render) {
        if ((mesh_render.render_groups & render_camera.render_groups) == 0)
            return;

        // Check visibility.
        if (world.has_component<scene::bounding_box>(entity))
        {
            auto& bounding_box = world.component<scene::bounding_box>(entity);
            if (!bounding_box.visible())
                return;
        }

        for (std::size_t i = 0; i < mesh_render.materials.size(); ++i)
        {
            render_item item = {};
            item.vertex_buffers = mesh_render.vertex_buffers.data();
            item.index_buffer = mesh_render.index_buffer;
            item.index_start = mesh_render.submeshes[i].index_start;
            item.index_end = mesh_render.submeshes[i].index_end;
            item.vertex_base = mesh_render.submeshes[i].vertex_base;
            item.additional_parameters = mesh_render.materials[i].parameters.data();
            item.scissor = mesh_render.materials[i].scissor;

            if (mesh_render.object_parameter != nullptr)
                item.object_parameter = mesh_render.object_parameter->interface();

            mesh_render.materials[i].pipeline->add_item(item);

            render_pipelines.insert(mesh_render.materials[i].pipeline);
        }
    });

    command->clear_render_target(render_camera.render_target(), {0.0f, 0.0f, 0.0f, 1.0f});
    command->clear_depth_stencil(render_camera.depth_stencil_buffer());

    for (auto& pipeline : render_pipelines)
    {
        pipeline->camera_parameter(render_camera.pipeline_parameter()->interface());
        pipeline->light_parameter(m_light_parameter->interface());
        pipeline->render_target(
            render_camera.render_target(),
            render_camera.render_target_resolve(),
            render_camera.depth_stencil_buffer());
        pipeline->render(command);
    }

    if (camera_entity != m_editor_camera)
    {
        // Render sky.
        m_sky_pipeline->camera_parameter(render_camera.pipeline_parameter()->interface());
        m_sky_pipeline->light_parameter(m_light_parameter->interface());
        m_sky_pipeline->sky_parameter(m_sky_parameter->interface());
        m_sky_pipeline->render_target(
            render_camera.render_target(),
            render_camera.render_target_resolve(),
            render_camera.depth_stencil_buffer());
        m_sky_pipeline->render(command);
    }

    rhi::renderer().execute(command);
}

void graphics::render_shadow_map(
    ecs::entity light_entity,
    const std::array<math::float4, 8>& frustum_vertices,
    render_command_interface* command)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();

    auto& light_transform = world.component<scene::transform>(light_entity);
    math::float4x4_simd to_world = math::simd::load(light_transform.to_world());
    math::float4x4_simd to_light = math::matrix_simd::inverse_transform(to_world);

    math::float4_simd aabb_min = math::simd::set(std::numeric_limits<float>::max());
    math::float4_simd aabb_max = math::simd::set(std::numeric_limits<float>::min());
    for (std::size_t i = 0; i < 8; ++i)
    {
        math::float4_simd v = math::simd::load(frustum_vertices[i]);
        v = math::matrix_simd::mul(v, to_light);
        aabb_min = math::simd::min(aabb_min, v);
        aabb_max = math::simd::max(aabb_max, v);
    }

    math::float4_simd range = math::vector_simd::sub(aabb_max, aabb_min);
    math::float4x4_simd vp = math::matrix_simd::orthographic(
        math::simd::get<0>(range),
        math::simd::get<1>(range),
        -100.0f,
        1000.0f);
    // math::float4x4_simd vp = math::matrix_simd::orthographic(20.0f, 20.0f, 0.01f, 1000.0f);
    vp = math::matrix_simd::mul(to_light, vp);

    math::float4x4 view_projection;
    math::simd::store(vp, view_projection);

    scene.frustum_culling(math::utility::frustum_planes(view_projection));

    world.view<mesh_render>().each([&, this](ecs::entity entity, mesh_render& mesh_render) {
        // if ((mesh_render.render_groups & render_camera.render_groups) == 0)
        //     return;

        // Check visibility.
        // if (world.has_component<scene::bounding_box>(entity))
        // {
        //     auto& bounding_box = world.component<scene::bounding_box>(entity);
        //     if (!bounding_box.visible())
        //         return;
        // }

        for (std::size_t i = 0; i < mesh_render.materials.size(); ++i)
        {
            render_item item = {};
            if (mesh_render.object_parameter != nullptr)
                item.object_parameter = mesh_render.object_parameter->interface();
            else
                continue;

            item.vertex_buffers = mesh_render.vertex_buffers.data();
            item.index_buffer = mesh_render.index_buffer;
            item.index_start = mesh_render.submeshes[i].index_start;
            item.index_end = mesh_render.submeshes[i].index_end;
            item.vertex_base = mesh_render.submeshes[i].vertex_base;

            m_shadow_pipeline->add_item(item);
        }
    });

    auto& light = world.component<directional_light>(light_entity);

    auto shadow_map = light.shadow();
    command->clear_depth_stencil(shadow_map->depth_buffer());

    shadow_map_pipeline_parameter* shadow_parameter = nullptr;
    if (m_shadow_parameter_counter >= m_shadow_parameter_pool.size())
    {
        m_shadow_parameter_pool.emplace_back(std::make_unique<shadow_map_pipeline_parameter>());
        shadow_parameter = m_shadow_parameter_pool.back().get();
    }
    else
    {
        shadow_parameter = m_shadow_parameter_pool[m_shadow_parameter_counter].get();
    }
    shadow_parameter->light_view_projection(view_projection);
    shadow_map->parameter(shadow_parameter);
    ++m_shadow_parameter_counter;

    m_shadow_pipeline->shadow(shadow_map);
    m_shadow_pipeline->render(command);
}

void graphics::present()
{
    rhi::renderer().present();
    m_debug->next_frame();
}
} // namespace ash::graphics