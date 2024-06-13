#include "light_viewer.hpp"
#include "core/event.hpp"
#include "core/relation.hpp"
#include "core/timer.hpp"
#include "graphics/camera.hpp"
#include "graphics/camera_frustum.hpp"
#include "graphics/graphics.hpp"
#include "graphics/graphics_event.hpp"
#include "graphics/mesh_render.hpp"
#include "graphics/rhi.hpp"
#include "scene/scene.hpp"
#include "task/task_manager.hpp"
#include "window/window.hpp"

namespace violet::sample
{
light_viewer::light_viewer() : core::system_base("light_viewer")
{
}

bool light_viewer::initialize(const dictionary& config)
{
    initialize_graphics_resource();
    initialize_task();
    initialize_scene();

    system<core::event>().subscribe<graphics::event_render_extent_change>(
        "sample_module",
        [this](std::uint32_t width, std::uint32_t height) { resize_camera(width, height); });

    return true;
}

void light_viewer::initialize_graphics_resource()
{
    // Initialize cube mesh.
    m_cube_mesh_data = graphics::geometry::box(1.0f, 1.0f, 1.0f);
    m_cube_positon_buffer = graphics::rhi::make_vertex_buffer(
        m_cube_mesh_data.position.data(),
        m_cube_mesh_data.position.size());
    m_cube_normal_buffer = graphics::rhi::make_vertex_buffer(
        m_cube_mesh_data.normal.data(),
        m_cube_mesh_data.normal.size());
    m_cube_index_buffer = graphics::rhi::make_index_buffer(
        m_cube_mesh_data.indices.data(),
        m_cube_mesh_data.indices.size());

    // Initialize sphere mesh.
    m_sphere_mesh_data = graphics::geometry::shpere(1.0f, 40, 20);
    m_sphere_positon_buffer = graphics::rhi::make_vertex_buffer(
        m_sphere_mesh_data.position.data(),
        m_sphere_mesh_data.position.size());
    m_sphere_normal_buffer = graphics::rhi::make_vertex_buffer(
        m_sphere_mesh_data.normal.data(),
        m_sphere_mesh_data.normal.size());
    m_sphere_index_buffer = graphics::rhi::make_index_buffer(
        m_sphere_mesh_data.indices.data(),
        m_sphere_mesh_data.indices.size());

    // Initialize blinn phong pipeline.
    m_material = std::make_unique<graphics::blinn_phong_material_pipeline_parameter>();
    m_material->diffuse(math::float3{1.0f, 1.0f, 1.0f});
    m_material->fresnel(math::float3{0.01f, 0.01f, 0.01f});
    m_material->roughness(0.2f);
    m_pipeline = std::make_unique<graphics::blinn_phong_pipeline>();

    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    // Initialize main camera.
    m_camera = world.create("main camera");
    world.add<core::link, graphics::camera, scene::transform>(m_camera);
    auto& camera = world.component<graphics::camera>(m_camera);
    camera.perspective(math::to_radians(45.0f), 0.03f, 100.0f);
    // camera.orthographic(10.0f, 0.03f, 1000.0f);

    auto& transform = world.component<scene::transform>(m_camera);
    transform.position(math::float3{0.0f, 0.0f, -38.0f});
    relation.link(m_camera, scene.root());

    auto& graphics = system<graphics::graphics>();
    auto extent = graphics.render_extent();
    resize_camera(extent.width, extent.height);

    graphics.game_camera(m_camera);
}

void light_viewer::initialize_task()
{
    auto& task = system<task::task_manager>();
    auto tick_task = task.schedule("bvh tick", [this]() {
        update_camera();
        system<scene::scene>().sync_local();

        // debug();
    });

    tick_task->add_dependency(*task.find(task::TASK_GAME_LOGIC_START));
    task.find(task::TASK_GAME_LOGIC_END)->add_dependency(*tick_task);
}

void light_viewer::initialize_scene()
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    {
        // Light.
        for (std::size_t i = 0; i < 2; ++i)
        {
            ecs::entity light = world.create("light");
            world.add<scene::transform, core::link, graphics::directional_light>(light);

            auto& transform = world.component<scene::transform>(light);
            transform.position(math::float3{0.0f, 50.0f, 0.0f});
            transform.rotation_euler(
                math::float3{math::to_radians(60.0f), math::to_radians(i * 20.0f), 0.0f});

            auto& directional_light = world.component<graphics::directional_light>(light);
            directional_light.color(math::float3{0.5f, 0.5f, 0.5f});
            relation.link(light, scene.root());

            m_lights.push_back(light);
        }
    }

    {
        // Cube.
        float x = -10.0f;
        float z = -10.0f;

        for (std::size_t i = 0; i < 10; ++i)
        {
            for (std::size_t j = 0; j < 10; ++j)
            {
                ecs::entity cube = world.create(std::format("cube {}-{}", i, j));
                world.add<scene::transform, scene::bounding_box, graphics::mesh_render, core::link>(
                    cube);

                auto& mesh = world.component<graphics::mesh_render>(cube);
                mesh.vertex_buffers = {m_cube_positon_buffer.get(), m_cube_normal_buffer.get()};
                mesh.index_buffer = m_cube_index_buffer.get();
                mesh.object_parameter = std::make_unique<graphics::object_pipeline_parameter>();

                graphics::material material = {};
                material.pipeline = m_pipeline.get();
                material.parameter = m_material->interface();
                mesh.materials.push_back(material);
                mesh.submeshes.push_back(graphics::submesh{0, m_cube_mesh_data.indices.size(), 0});

                auto& transform = world.component<scene::transform>(cube);
                transform.position(math::float3{x + i * 2.0f, 0.5f, z + j * 2.0f});
                scene.sync_local();

                auto& bounding_box = world.component<scene::bounding_box>(cube);
                bounding_box.aabb(m_cube_mesh_data.position, transform.to_world(), true);

                relation.link(cube, scene.root());

                m_cubes.push_back(cube);
            }
        }
    }

    {
        // Sphere.
        float x = -10.0f;
        float y = 2.0f;

        for (std::size_t i = 0; i < 10; ++i)
        {
            for (std::size_t j = 0; j < 10; ++j)
            {
                ecs::entity sphere = world.create(std::format("sphere {}-{}", i, j));
                world.add<scene::transform, scene::bounding_box, graphics::mesh_render, core::link>(
                    sphere);

                auto& mesh = world.component<graphics::mesh_render>(sphere);
                mesh.vertex_buffers = {m_sphere_positon_buffer.get(), m_sphere_normal_buffer.get()};
                mesh.index_buffer = m_sphere_index_buffer.get();
                mesh.object_parameter = std::make_unique<graphics::object_pipeline_parameter>();

                graphics::material material = {};
                material.pipeline = m_pipeline.get();
                material.parameter = m_material->interface();
                mesh.materials.push_back(material);
                mesh.submeshes.push_back(
                    graphics::submesh{0, m_sphere_mesh_data.indices.size(), 0});

                auto& transform = world.component<scene::transform>(sphere);
                transform.position(math::float3{x + i * 2.0f, y + j * 2.0f, 0.0f});
                scene.sync_local();

                auto& bounding_box = world.component<scene::bounding_box>(sphere);
                bounding_box.aabb(m_sphere_mesh_data.position, transform.to_world(), true);

                relation.link(sphere, scene.root());

                m_sphere.push_back(sphere);
            }
        }
    }

    {
        // Plane.
        m_plane = world.create("plane");
        world.add<scene::transform, scene::bounding_box, graphics::mesh_render, core::link>(
            m_plane);

        auto& mesh = world.component<graphics::mesh_render>(m_plane);
        mesh.vertex_buffers = {m_cube_positon_buffer.get(), m_cube_normal_buffer.get()};
        mesh.index_buffer = m_cube_index_buffer.get();
        mesh.object_parameter = std::make_unique<graphics::object_pipeline_parameter>();

        graphics::material material = {};
        material.pipeline = m_pipeline.get();
        material.parameter = m_material->interface();
        mesh.materials.push_back(material);
        mesh.submeshes.push_back(graphics::submesh{0, m_cube_mesh_data.indices.size(), 0});

        auto& transform = world.component<scene::transform>(m_plane);
        transform.position(math::float3{0.0f, 0.0f, 0.0f});
        transform.scale(math::float3{500.0f, 0.05f, 500.0f});
        scene.sync_local();

        auto& bounding_box = world.component<scene::bounding_box>(m_plane);
        bounding_box.aabb(m_cube_mesh_data.position, transform.to_world(), true);

        relation.link(m_plane, scene.root());
    }

    scene.sync_local();
}

void light_viewer::update_camera()
{
    auto& world = system<ecs::world>();
    auto& keyboard = system<window::window>().keyboard();
    auto& mouse = system<window::window>().mouse();

    float delta = system<core::timer>().frame_delta();

    auto& camera_transform = world.component<scene::transform>(m_camera);
    if (keyboard.key(window::KEYBOARD_KEY_1).release())
    {
        if (mouse.mode() == window::MOUSE_MODE_RELATIVE)
        {
            mouse.mode(window::MOUSE_MODE_ABSOLUTE);
        }
        else
        {
            mouse.mode(window::MOUSE_MODE_RELATIVE);
            m_camera_rotation = math::euler::rotation_quaternion(camera_transform.rotation());
        }
    }

    if (mouse.mode() == window::MOUSE_MODE_RELATIVE)
    {
        m_camera_rotation[1] += mouse.x() * m_rotate_speed * delta;
        m_camera_rotation[0] += mouse.y() * m_rotate_speed * delta;
        m_camera_rotation[0] = std::clamp(m_camera_rotation[0], -math::PI_PIDIV2, math::PI_PIDIV2);
        camera_transform.rotation_euler(m_camera_rotation);
    }

    float x = 0, z = 0;
    if (keyboard.key(window::KEYBOARD_KEY_W).down())
        z += 1.0f;
    if (keyboard.key(window::KEYBOARD_KEY_S).down())
        z -= 1.0f;
    if (keyboard.key(window::KEYBOARD_KEY_D).down())
        x += 1.0f;
    if (keyboard.key(window::KEYBOARD_KEY_A).down())
        x -= 1.0f;

    math::vector4 s = math::simd::load(camera_transform.scale());
    math::vector4 r = math::simd::load(camera_transform.rotation());
    math::vector4 t = math::simd::load(camera_transform.position());

    math::matrix4 affine = math::matrix_simd::affine_transform(s, r, t);
    math::vector4 forward =
        math::simd::set(x * m_move_speed * delta, 0.0f, z * m_move_speed * delta, 0.0f);
    forward = math::matrix_simd::mul(forward, affine);

    camera_transform.position(math::vector_simd::add(forward, t));
}

void light_viewer::resize_camera(std::uint32_t width, std::uint32_t height)
{
    auto& world = system<ecs::world>();
    auto& camera = world.component<graphics::camera>(m_camera);
    camera.render_groups |= graphics::RENDER_GROUP_DEBUG;

    graphics::render_target_desc render_target = {};
    render_target.width = width;
    render_target.height = height;
    render_target.format = graphics::rhi::back_buffer_format();
    render_target.samples = 4;
    m_render_target = graphics::rhi::make_render_target(render_target);
    camera.render_target(m_render_target.get());

    graphics::depth_stencil_buffer_desc depth_stencil_buffer = {};
    depth_stencil_buffer.width = width;
    depth_stencil_buffer.height = height;
    depth_stencil_buffer.format = graphics::RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_buffer.samples = 4;
    m_depth_stencil_buffer = graphics::rhi::make_depth_stencil_buffer(depth_stencil_buffer);
    camera.depth_stencil_buffer(m_depth_stencil_buffer.get());
}

void light_viewer::debug()
{
    /*auto& debug_draw = system<graphics::graphics>().debug();

    auto draw_frustum = [&](const auto* frustum) {
        debug_draw.draw_line(frustum[0], frustum[1], math::float3{1.0f, 0.0f, 1.0f});
        debug_draw.draw_line(frustum[1], frustum[3], math::float3{1.0f, 0.0f, 1.0f});
        debug_draw.draw_line(frustum[3], frustum[2], math::float3{1.0f, 0.0f, 1.0f});
        debug_draw.draw_line(frustum[2], frustum[0], math::float3{1.0f, 0.0f, 1.0f});

        debug_draw.draw_line(frustum[2], frustum[6], math::float3{1.0f, 0.0f, 1.0f});
        debug_draw.draw_line(frustum[3], frustum[7], math::float3{1.0f, 0.0f, 1.0f});
        debug_draw.draw_line(frustum[0], frustum[4], math::float3{1.0f, 0.0f, 1.0f});
        debug_draw.draw_line(frustum[1], frustum[5], math::float3{1.0f, 0.0f, 1.0f});

        debug_draw.draw_line(frustum[4], frustum[5], math::float3{1.0f, 1.0f, 1.0f});
        debug_draw.draw_line(frustum[5], frustum[7], math::float3{1.0f, 1.0f, 1.0f});
        debug_draw.draw_line(frustum[7], frustum[6], math::float3{1.0f, 1.0f, 1.0f});
        debug_draw.draw_line(frustum[6], frustum[4], math::float3{1.0f, 1.0f, 1.0f});
    };

    auto& world = system<ecs::world>();

    auto& camera_transform = world.component<scene::transform>(m_camera);
    auto& camera = world.component<graphics::camera>(m_camera);
    math::matrix4 camera_v =
        math::matrix_simd::inverse_transform(math::simd::load(camera_transform.to_world()));
    math::matrix4 camera_p = math::simd::load(camera.projection());
    math::matrix4 camera_vp = math::matrix_simd::mul(camera_v, camera_p);
    math::float4x4 camera_view_projection;
    math::simd::store(camera_vp, camera_view_projection);

    std::size_t shadow_cascade_count = 4;
    math::float4 shadow_cascade_splits = {0.067f, 0.133f, 0.267f, 0.533f};

    auto camera_frustum_vertices = graphics::camera_frustum::vertices(camera_view_projection);

    std::vector<math::float3> frustum_cascade_vertices((shadow_cascade_count + 1) * 4);
    for (std::size_t i = 0; i < 4; ++i)
    {
        // Near plane vertices.
        frustum_cascade_vertices[i] = camera_frustum_vertices[i];
        // Far plane vertices.
        frustum_cascade_vertices[frustum_cascade_vertices.size() - 4 + i] =
            camera_frustum_vertices[i + 4];
    }

    for (std::size_t i = 1; i < shadow_cascade_count; ++i)
    {
        for (std::size_t j = 0; j < 4; ++j)
        {
            math::vector4 n = math::simd::load(camera_frustum_vertices[j]);
            math::vector4 f = math::simd::load(camera_frustum_vertices[j + 4]);

            math::vector4 m = math::vector_simd::lerp(n, f, shadow_cascade_splits[i - 1]);
            math::simd::store(m, frustum_cascade_vertices[i * 4 + j]);
        }
    }

    auto& light_transform = world.component<scene::transform>(m_light);
    math::matrix4 light_to_world = math::simd::load(light_transform.to_world());
    math::matrix4 light_v = math::matrix_simd::inverse_transform(light_to_world);
    for (std::size_t cascade = 0; cascade < shadow_cascade_count; ++cascade)
    {
        auto cascade_vertices = frustum_cascade_vertices.data() + cascade * 4;
        draw_frustum(cascade_vertices);

        math::vector4 aabb_min = math::simd::set(std::numeric_limits<float>::max());
        math::vector4 aabb_max = math::simd::set(std::numeric_limits<float>::lowest());
        for (std::size_t i = 0; i < 8; ++i)
        {
            math::vector4 v = math::simd::load(cascade_vertices[i], 1.0f);
            v = math::matrix_simd::mul(v, light_v);
            aabb_min = math::simd::min(aabb_min, v);
            aabb_max = math::simd::max(aabb_max, v);
        }

        math::matrix4 light_vp = math::matrix_simd::orthographic(
            math::simd::get<0>(aabb_min),
            math::simd::get<0>(aabb_max),
            math::simd::get<1>(aabb_min),
            math::simd::get<1>(aabb_max),
            math::simd::get<2>(aabb_min),
            math::simd::get<2>(aabb_max));
        light_vp = math::matrix_simd::mul(light_v, light_vp);
        math::float4x4 light_view_projection;
        math::simd::store(light_vp, light_view_projection);

        auto light_frustum_vertices = graphics::camera_frustum::vertices(light_view_projection);
        draw_frustum(light_frustum_vertices.data());
    }*/
}
} // namespace violet::sample