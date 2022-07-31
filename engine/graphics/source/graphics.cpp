#include "graphics/graphics.hpp"
#include "graphics/blinn_phong_pipeline.hpp"
#include "graphics/camera.hpp"
#include "graphics/camera_frustum.hpp"
#include "graphics/compute_pipeline.hpp"
#include "graphics/graphics_event.hpp"
#include "graphics/graphics_task.hpp"
#include "graphics/light.hpp"
#include "graphics/rhi.hpp"
#include "graphics/shadow_map.hpp"
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
      m_shadow_map_counter(0),
      m_shadow_cascade_count(4),
      m_shadow_cascade_splits{0.067f, 0.133f, 0.267f, 0.533f}
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
    m_light_parameter->ambient_light(math::float3{0.5f, 0.5f, 0.5f});

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

void graphics::ambient_light(const math::float3& ambient_light)
{
    m_light_parameter->ambient_light(ambient_light);
}

void graphics::shadow_cascade(std::size_t cascade_count, const math::float4& cascade_splits)
{
    ASH_ASSERT(cascade_count < light_pipeline_parameter::MAX_CASCADED_COUNT);

    m_shadow_cascade_count = cascade_count;
    m_shadow_cascade_splits = cascade_splits;
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

    m_shadow_map_counter = 0;
}

void graphics::render_camera(ecs::entity camera_entity)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();

    auto& render_camera = world.component<camera>(camera_entity);
    auto& transform = world.component<scene::transform>(camera_entity);

    // Update camera data.
    math::float4x4_simd camera_v =
        math::matrix_simd::inverse_transform(math::simd::load(transform.to_world()));
    math::float4x4_simd camera_p = math::simd::load(render_camera.projection());
    math::float4x4_simd camera_vp = math::matrix_simd::mul(camera_v, camera_p);

    render_camera.pipeline_parameter()->position(transform.position());
    render_camera.pipeline_parameter()->direction(transform.forward());

    math::float4x4 camera_view, camera_view_projection;
    math::simd::store(camera_v, camera_view);
    math::simd::store(camera_vp, camera_view_projection);
    render_camera.pipeline_parameter()->view(camera_view);
    render_camera.pipeline_parameter()->projection(render_camera.projection());
    render_camera.pipeline_parameter()->view_projection(camera_view_projection);

    // Render.
    auto command = rhi::renderer().allocate_command();

    // Update light data and render shadow map.
    render_shadow(render_camera.near_z(), render_camera.far_z(), camera_view_projection, command);

    // Frustum culling.
    scene.frustum_culling(camera_frustum::planes(camera_view_projection));

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

void graphics::render_shadow(
    float camera_near_z,
    float camera_far_z,
    const math::float4x4& camera_view_projection,
    render_command_interface* command)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();

    auto frustum_vertices = camera_frustum::vertices_vec4(camera_view_projection);
    std::vector<math::float4> frustum_cascade_vertices((m_shadow_cascade_count + 1) * 4);
    for (std::size_t i = 0; i < 4; ++i)
    {
        // Near plane vertices.
        frustum_cascade_vertices[i] = frustum_vertices[i];
        // Far plane vertices.
        frustum_cascade_vertices[frustum_cascade_vertices.size() - 4 + i] = frustum_vertices[i + 4];
    }

    for (std::size_t i = 1; i < m_shadow_cascade_count; ++i)
    {
        for (std::size_t j = 0; j < 4; ++j)
        {
            math::float4_simd n = math::simd::load(frustum_vertices[j]);
            math::float4_simd f = math::simd::load(frustum_vertices[j + 4]);

            math::float4_simd m = math::vector_simd::lerp(n, f, m_shadow_cascade_splits[i - 1]);
            math::simd::store(m, frustum_cascade_vertices[i * 4 + j]);
        }
    }

    std::size_t directional_light_counter = 0;
    world.view<directional_light, scene::transform>().each([&](directional_light& light,
                                                               scene::transform& transform) {
        math::float4x4_simd light_v =
            math::matrix_simd::inverse_transform(math::simd::load(transform.to_world()));
        math::float4x4 light_view;
        math::simd::store(light_v, light_view);

        std::array<math::float4, 4> cascade_scale;
        std::array<math::float4, 4> cascade_offset;
        for (std::size_t i = 0; i < 4; ++i)
        {
            math::float4x4 light_projection;
            shadow_map* shadow_map = render_shadow_cascade(
                light_view,
                frustum_cascade_vertices.data() + i * 4,
                command,
                light_projection);

            cascade_scale[i] =
                {light_projection[0][0], light_projection[1][1], light_projection[2][2], 1.0f};
            cascade_offset[i] = light_projection[3];

            m_light_parameter->shadow_map(directional_light_counter, i, shadow_map->depth_buffer());
        }
        m_light_parameter
            ->shadow(directional_light_counter, light_view, cascade_scale, cascade_offset);

        m_light_parameter->directional_light(
            directional_light_counter,
            light.color(),
            transform.forward(),
            true,
            directional_light_counter);

        ++directional_light_counter;
    });
    m_light_parameter->shadow_count(directional_light_counter, m_shadow_cascade_count);

    math::float4 shadow_cascade_splits;
    math::simd::store(
        math::vector_simd::lerp(
            math::simd::set(camera_near_z),
            math::simd::set(camera_far_z),
            math::simd::load(m_shadow_cascade_splits)),
        shadow_cascade_splits);
    m_light_parameter->shadow_cascade_depths(shadow_cascade_splits);

    m_light_parameter->directional_light_count(directional_light_counter);
}

shadow_map* graphics::render_shadow_cascade(
    const math::float4x4& light_view,
    const math::float4* frustum_vertex,
    render_command_interface* command,
    math::float4x4& light_projection)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();

    math::float4_simd aabb_min = math::simd::set(std::numeric_limits<float>::max());
    math::float4_simd aabb_max = math::simd::set(std::numeric_limits<float>::lowest());

    math::float4x4_simd light_v = math::simd::load(light_view);

    for (std::size_t i = 0; i < 8; ++i)
    {
        math::float4_simd v = math::simd::load(frustum_vertex[i]);
        v = math::matrix_simd::mul(v, light_v);
        aabb_min = math::simd::min(aabb_min, v);
        aabb_max = math::simd::max(aabb_max, v);
    }

    math::float4x4_simd light_p = math::matrix_simd::orthographic(
        math::simd::get<0>(aabb_min),
        math::simd::get<0>(aabb_max),
        math::simd::get<1>(aabb_min),
        math::simd::get<1>(aabb_max),
        math::simd::get<2>(aabb_min) - 100.0f,
        math::simd::get<2>(aabb_max));
    math::simd::store(light_p, light_projection);

    math::float4x4_simd light_vp = math::matrix_simd::mul(light_v, light_p);

    math::float4x4 view_projection;
    math::simd::store(light_vp, view_projection);

    scene.frustum_culling(camera_frustum::planes(view_projection));
    world.view<mesh_render>().each([&, this](ecs::entity entity, mesh_render& mesh_render) {
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

    shadow_map* shadow = allocate_shadow_map();
    shadow->light_view_projection(view_projection);

    m_shadow_pipeline->shadow(shadow);
    m_shadow_pipeline->render(command);
    m_shadow_pipeline->clear();

    return shadow;
}

void graphics::present()
{
    rhi::renderer().present();
    m_debug->next_frame();
}

shadow_map* graphics::allocate_shadow_map()
{
    shadow_map* shadow = nullptr;
    if (m_shadow_map_counter >= m_shadow_map_pool.size())
    {
        m_shadow_map_pool.emplace_back(std::make_unique<shadow_map>(2048));
        shadow = m_shadow_map_pool.back().get();
    }
    else
    {
        shadow = m_shadow_map_pool[m_shadow_map_counter].get();
    }
    ++m_shadow_map_counter;

    return shadow;
}
} // namespace ash::graphics