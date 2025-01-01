#include "bvh_viewer.hpp"
#include "core/relation.hpp"
#include "core/timer.hpp"
#include "graphics/blinn_phong_pipeline.hpp"
#include "graphics/camera.hpp"
#include "graphics/graphics.hpp"
#include "graphics/graphics_event.hpp"
#include "graphics/mesh_render.hpp"
#include "graphics/rhi.hpp"
#include "scene/bvh_tree.hpp"
#include "scene/scene.hpp"
#include "task/task_manager.hpp"
#include "ui/ui.hpp"
#include "window/window.hpp"
#include <random>

namespace violet

{
bvh_viewer::bvh_viewer() : core::system_base("bvh_viewer")
{
}

bool bvh_viewer::initialize(const dictionary& config)
{
    initialize_graphics_resource();
    initialize_ui();
    initialize_task();

    system<core::event>().subscribe<graphics::event_render_extent_change>(
        "sample_module",
        [this](std::uint32_t width, std::uint32_t height) { resize_camera(width, height); });

    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    m_light = world.create("light");
    world.add<scene::transform, core::link, graphics::directional_light>(m_light);
    auto& directional_light = world.component<graphics::directional_light>(m_light);
    directional_light.color(math::float3{0.5f, 0.5f, 0.5f});
    relation.link(m_light, scene.root());

    return true;
}

void bvh_viewer::initialize_graphics_resource()
{
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
    m_cube_material = std::make_unique<graphics::blinn_phong_material_pipeline_parameter>();
    m_cube_material->diffuse(math::float3{1.0f, 1.0f, 1.0f});
    m_pipeline = std::make_unique<graphics::blinn_phong_pipeline>();

    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    {
        // Initialize main camera.
        m_camera = world.create();
        world.add<core::link, graphics::camera, scene::transform>(m_camera);

        auto& transform = world.component<scene::transform>(m_camera);
        transform.position(math::float3{0.0f, 0.0f, -38.0f});
        relation.link(m_camera, scene.root());

        auto& graphics = system<graphics::graphics>();
        auto extent = graphics.render_extent();
        resize_camera(extent.width, extent.height);

        graphics.game_camera(m_camera);
    }

    {
        // Initialize small camera.
        m_small_camera = world.create();
        world.add<core::link, graphics::camera, scene::transform>(m_small_camera);
        auto& camera = world.component<graphics::camera>(m_small_camera);
        camera.render_groups |= graphics::RENDER_GROUP_DEBUG;
        camera.render_groups ^= graphics::RENDER_GROUP_UI;

        auto& transform = world.component<scene::transform>(m_small_camera);
        transform.position(math::float3{0.0f, 0.0f, -50.0f});
        relation.link(m_small_camera, scene.root());

        graphics::render_target_desc render_target = {};
        render_target.width = 400.0f;
        render_target.height = 400.0f;
        render_target.format = graphics::rhi::back_buffer_format();
        render_target.samples = 4;
        m_small_render_target = graphics::rhi::make_render_target(render_target);
        camera.render_target(m_small_render_target.get());

        graphics::render_target_desc render_target_resolve = {};
        render_target_resolve.width = 400.0f;
        render_target_resolve.height = 400.0f;
        render_target_resolve.format = graphics::rhi::back_buffer_format();
        render_target_resolve.samples = 1;
        m_small_render_target_resolve = graphics::rhi::make_render_target(render_target_resolve);
        camera.render_target_resolve(m_small_render_target_resolve.get());

        graphics::depth_stencil_buffer_desc depth_stencil_buffer = {};
        depth_stencil_buffer.width = 400.0f;
        depth_stencil_buffer.height = 400.0f;
        depth_stencil_buffer.format = graphics::RESOURCE_FORMAT_D24_UNORM_S8_UINT;
        depth_stencil_buffer.samples = 4;
        m_small_depth_stencil_buffer =
            graphics::rhi::make_depth_stencil_buffer(depth_stencil_buffer);
        camera.depth_stencil_buffer(m_small_depth_stencil_buffer.get());
    }
}

void bvh_viewer::initialize_task()
{
    auto& task = system<task::task_manager>();

    auto tick_task = task.schedule("bvh tick", [this]() {
        update_camera();
        move_cube();
        draw_aabb();

        system<graphics::graphics>().render(m_small_camera);
    });

    tick_task->add_dependency(*task.find(task::TASK_GAME_LOGIC_START));
    task.find(task::TASK_GAME_LOGIC_END)->add_dependency(*tick_task);
}

void bvh_viewer::initialize_ui()
{
    auto& ui = system<ui::ui>();
    m_add_button = std::make_unique<ui::button>("Add Cube", ui.theme<ui::button_theme>("dark"));
    m_add_button->width(200.0f);
    m_add_button->height(40.0f);
    m_add_button->on_mouse_press = [this](window::mouse_key key, int x, int y) {
        add_cube(true);
        return false;
    };
    ui.root()->add(m_add_button.get());

    m_remove_button =
        std::make_unique<ui::button>("Remove Cube", ui.theme<ui::button_theme>("dark"));
    m_remove_button->width(200.0f);
    m_remove_button->height(40.0f);
    m_remove_button->on_mouse_press = [this](window::mouse_key key, int x, int y) {
        remove_cube();
        return false;
    };
    ui.root()->add(m_remove_button.get());

    m_small_view = std::make_unique<ui::image>(m_small_render_target_resolve.get());
    m_small_view->position_type(ui::LAYOUT_POSITION_TYPE_ABSOLUTE);
    m_small_view->position(0.0f, ui::LAYOUT_EDGE_RIGHT);
    m_small_view->position(0.0f, ui::LAYOUT_EDGE_BOTTOM);
    ui.root()->add(m_small_view.get());
}

void bvh_viewer::move_cube()
{
    auto& world = system<ecs::world>();

    float delta = system<core::timer>().frame_delta();
    for (std::size_t i = 0; i < m_cubes.size(); ++i)
    {
        auto& transform = world.component<scene::transform>(m_cubes[i]);

        math::float3 position = transform.position();
        if (position[0] > 10.0f || position[0] < -10.0f || position[1] > 10.0f ||
            position[1] < -10.0f)
        {
            m_move_direction[i][0] = -m_move_direction[i][0];
            m_move_direction[i][1] = -m_move_direction[i][1];
        }
        position[0] += m_move_direction[i][0] * delta;
        position[1] += m_move_direction[i][1] * delta;

        transform.position(position);
        transform.rotation(math::quaternion::rotation_euler(position));
    }

    system<scene::scene>().sync_local();
}

void bvh_viewer::draw_aabb()
{
    auto& world = system<ecs::world>();

    auto& drawer = system<graphics::graphics>().debug();
    world.view<scene::transform, scene::bounding_box>().each(
        [&drawer](scene::transform& transform, scene::bounding_box& bounding_box) {
            if (bounding_box.visible())
                drawer.draw_aabb(
                    bounding_box.aabb().min,
                    bounding_box.aabb().max,
                    math::float3{0.0f, 1.0f, 0.0f});
            else
                drawer.draw_aabb(
                    bounding_box.aabb().min,
                    bounding_box.aabb().max,
                    math::float3{1.0f, 0.0f, 0.0f});
        });
}

void bvh_viewer::update_camera()
{
    auto& world = system<ecs::world>();
    auto& keyboard = system<window::window>().keyboard();
    auto& mouse = system<window::window>().mouse();

    float delta = system<core::timer>().frame_delta();

    if (keyboard.key(window::KEYBOARD_KEY_1).release())
    {
        if (mouse.mode() == window::MOUSE_MODE_RELATIVE)
            mouse.mode(window::MOUSE_MODE_ABSOLUTE);
        else
            mouse.mode(window::MOUSE_MODE_RELATIVE);
    }

    auto& transform = world.component<scene::transform>(m_camera);
    if (mouse.mode() == window::MOUSE_MODE_RELATIVE)
    {
        m_heading += mouse.x() * m_rotate_speed * delta;
        m_pitch += mouse.y() * m_rotate_speed * delta;
        m_pitch = std::clamp(m_pitch, -math::PI_PIDIV2, math::PI_PIDIV2);
        transform.rotation_euler({m_pitch, m_heading, 0.0f});
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

    math::vector4 s = math::simd::load(transform.scale());
    math::vector4 r = math::simd::load(transform.rotation());
    math::vector4 t = math::simd::load(transform.position());

    math::matrix4 affine = math::matrix_simd::affine_transform(s, r, t);
    math::vector4 forward =
        math::simd::set(x * m_move_speed * delta, 0.0f, z * m_move_speed * delta, 0.0f);
    forward = math::matrix_simd::mul(forward, affine);

    transform.position(math::vector_simd::add(forward, t));
}

void bvh_viewer::resize_camera(std::uint32_t width, std::uint32_t height)
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

void bvh_viewer::add_cube(bool random)
{
    auto& world = system<ecs::world>();

    ecs::entity cube = world.create();
    world.add<scene::transform, scene::bounding_box, graphics::mesh_render, core::link>(cube);

    auto& mesh = world.component<graphics::mesh_render>(cube);
    mesh.vertex_buffers = {m_cube_positon_buffer.get(), m_cube_normal_buffer.get()};
    mesh.index_buffer = m_cube_index_buffer.get();
    mesh.object_parameter = std::make_unique<graphics::object_pipeline_parameter>();

    graphics::material material = {};
    material.pipeline = m_pipeline.get();
    material.parameter = m_cube_material->interface();
    mesh.materials.push_back(material);
    mesh.submeshes.push_back(graphics::submesh{0, 36, 0});

    auto& transform = world.component<scene::transform>(cube);

    static std::default_random_engine random_engine;
    if (random)
    {
        static std::uniform_real_distribution<float> pu(-10, 10);
        static std::uniform_real_distribution<float> ru(-math::PI, math::PI);
        transform.position(math::float3{pu(random_engine), pu(random_engine), 0.0f});
        transform.rotation_euler(
            math::float3{ru(random_engine), ru(random_engine), ru(random_engine)});
    }
    else
    {
        static float x = 0.0f;
        transform.position(math::float3{x, x, 0.0f});
        x += 1.5f;
    }

    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    auto& bounding_box = world.component<scene::bounding_box>(cube);
    bounding_box.aabb(m_cube_mesh_data.position, transform.to_world(), true);

    relation.link(cube, scene.root());
    scene.sync_local();

    m_cubes.push_back(cube);

    static std::uniform_real_distribution<float> tu(-1.0f, 1.0f);
    m_move_direction.emplace_back(
        math::float3{tu(random_engine), tu(random_engine), tu(random_engine)});
}

void bvh_viewer::remove_cube()
{
    if (m_cubes.empty())
        return;

    static std::default_random_engine random_engine;

    std::size_t index = random_engine() % m_cubes.size();

    std::swap(m_cubes[index], m_cubes.back());
    std::swap(m_move_direction[index], m_move_direction.back());

    system<core::relation>().unlink(m_cubes.back());
    system<ecs::world>().release(m_cubes.back());

    m_cubes.pop_back();
}
} // namespace violet