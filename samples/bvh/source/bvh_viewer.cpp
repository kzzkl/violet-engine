#include "bvh_viewer.hpp"
#include "core/relation.hpp"
#include "core/timer.hpp"
#include "graphics/graphics.hpp"
#include "graphics/mesh_render.hpp"
#include "graphics/rhi.hpp"
#include "graphics/standard_pipeline.hpp"
#include "scene/bvh_tree.hpp"
#include "scene/scene.hpp"
#include "task/task_manager.hpp"
#include "ui/ui.hpp"
#include "window/window.hpp"
#include <random>

namespace ash::sample
{
bvh_viewer::bvh_viewer() : core::system_base("bvh_viewer")
{
}

bool bvh_viewer::initialize(const dictionary& config)
{
    initialize_graphics_resource();
    initialize_ui();
    initialize_task();

    auto& world = system<ecs::world>();
    m_aabb_view = world.make_view<scene::transform, scene::bounding_box>();

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
    m_cube_material = std::make_unique<graphics::standard_material_pipeline_parameter>();
    m_cube_material->diffuse(math::float3{1.0f, 1.0f, 1.0f});
    m_pipeline = std::make_unique<graphics::standard_pipeline>();

    // Initialize camera.
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    m_camera = world.create();
    world.add<core::link, graphics::camera, scene::transform>(m_camera);
    auto& camera = world.component<graphics::camera>(m_camera);
    camera.field_of_view(math::to_radians(30.0f));
    camera.clipping_planes(0.01f, 1000.0f);

    auto& transform = world.component<scene::transform>(m_camera);
    transform.position = {0.0f, 0.0f, -38.0f};
    transform.world_matrix = math::matrix_plain::affine_transform(
        transform.scaling,
        transform.rotation,
        transform.position);

    relation.link(m_camera, scene.root());

    auto& graphics = system<graphics::graphics>();
    auto extent = graphics.render_extent();
    resize_camera(extent.width, extent.height);

    graphics.game_camera(m_camera);
}

void bvh_viewer::initialize_task()
{
    auto& task = system<task::task_manager>();

    auto tick_task = task.schedule("bvh tick", [this]() {
        update_camera();
        system<scene::scene>().sync_local();

        draw_aabb();
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
}

void bvh_viewer::draw_aabb()
{
    auto& drawer = system<graphics::graphics>().debug();
    /*m_aabb_view->each([&drawer](scene::transform& transform, scene::bounding_box& bounding_box) {
        drawer.draw_aabb(bounding_box.min, bounding_box.max, math::float3{0.0f, 1.0f, 0.0f});
    });*/

    m_tree.print([&drawer](const scene::bounding_volume_aabb& aabb) {
        drawer.draw_aabb(aabb.min, aabb.max, math::float3{1.0f, 0.0f, 0.0f});
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

    if (keyboard.key(window::KEYBOARD_KEY_3).release())
    {
        static std::size_t index = 0;
        static std::vector<math::float3> colors = {
            math::float3{1.0f, 0.0f, 0.0f},
            math::float3{0.0f, 1.0f, 0.0f},
            math::float3{0.0f, 0.0f, 1.0f}
        };

        m_cube_material->diffuse(colors[index]);
        index = (index + 1) % colors.size();
    }

    auto& transform = world.component<scene::transform>(m_camera);
    if (mouse.mode() == window::MOUSE_MODE_RELATIVE)
    {
        m_heading += mouse.x() * m_rotate_speed * delta;
        m_pitch += mouse.y() * m_rotate_speed * delta;
        m_pitch = std::clamp(m_pitch, -math::PI_PIDIV2, math::PI_PIDIV2);
        transform.rotation = math::quaternion_plain::rotation_euler(m_heading, m_pitch, 0.0f);
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

    math::float4_simd s = math::simd::load(transform.scaling);
    math::float4_simd r = math::simd::load(transform.rotation);
    math::float4_simd t = math::simd::load(transform.position);

    math::float4x4_simd affine = math::matrix_simd::affine_transform(s, r, t);
    math::float4_simd forward =
        math::simd::set(x * m_move_speed * delta, 0.0f, z * m_move_speed * delta, 0.0f);
    forward = math::matrix_simd::mul(forward, affine);

    math::simd::store(math::vector_simd::add(forward, t), transform.position);
    transform.dirty = true;
}

void bvh_viewer::resize_camera(std::uint32_t width, std::uint32_t height)
{
    auto& world = system<ecs::world>();
    auto& camera = world.component<graphics::camera>(m_camera);
    camera.render_groups |= graphics::RENDER_GROUP_DEBUG;

    graphics::render_target_info render_target_info = {};
    render_target_info.width = width;
    render_target_info.height = height;
    render_target_info.format = graphics::rhi::back_buffer_format();
    render_target_info.samples = 4;
    m_render_target = graphics::rhi::make_render_target(render_target_info);
    camera.render_target(m_render_target.get());

    graphics::depth_stencil_buffer_info depth_stencil_buffer_info = {};
    depth_stencil_buffer_info.width = width;
    depth_stencil_buffer_info.height = height;
    depth_stencil_buffer_info.format = graphics::RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_buffer_info.samples = 4;
    m_depth_stencil_buffer = graphics::rhi::make_depth_stencil_buffer(depth_stencil_buffer_info);
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
    material.parameters = {mesh.object_parameter->interface(), m_cube_material->interface()};
    mesh.materials.push_back(material);
    mesh.submeshes.push_back(graphics::submesh{0, 36, 0});

    auto& transform = world.component<scene::transform>(cube);
    if (random)
    {
        static std::default_random_engine random_engine;
        static std::uniform_real_distribution<float> pu(-10, 10);
        static std::uniform_real_distribution<float> ru(-math::PI, math::PI);
        transform.position[0] = pu(random_engine);
        transform.position[1] = pu(random_engine);
        // transform.position[2] = pu(random_engine);
        transform.rotation = math::quaternion_plain::rotation_euler(
            ru(random_engine),
            ru(random_engine),
            ru(random_engine));
    }
    else
    {
        static float x = 0.0f;
        transform.position[0] = x;
        transform.position[1] = x;
        x += 1.5f;
    }
    transform.dirty = true;

    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    relation.link(cube, scene.root());
    scene.sync_local();

    auto& bounding_box = world.component<scene::bounding_box>(cube);
    bounding_box.aabb(m_cube_mesh_data.position, transform.world_matrix);

    std::size_t proxy_id = m_tree.add(bounding_box.aabb());
    m_cubes.push_back({cube, proxy_id});
}

void bvh_viewer::remove_cube()
{
    if (m_cubes.empty())
        return;

    static std::default_random_engine random_engine;

    std::size_t index = random_engine() % m_cubes.size();
    m_tree.remove(m_cubes[index].second);

    std::swap(m_cubes[index], m_cubes.back());

    system<core::relation>().unlink(m_cubes.back().first);
    system<ecs::world>().release(m_cubes.back().first);

    m_cubes.pop_back();
}
} // namespace ash::sample