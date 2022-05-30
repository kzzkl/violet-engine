#include "editor/sample.hpp"
#include "core/relation.hpp"
#include "graphics/geometry.hpp"
#include "graphics/graphics_event.hpp"
#include "scene/scene.hpp"

namespace ash::editor
{
bool test_module::initialize(const dictionary& config)
{
    auto& world = system<ecs::world>();
    auto& graphics = system<graphics::graphics>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();
    auto& physics = system<physics::physics>();

    // Create rigidbody shape.
    physics::collision_shape_desc desc;
    desc.type = physics::collision_shape_type::BOX;
    desc.box.length = 1.0f;
    desc.box.height = 1.0f;
    desc.box.width = 1.0f;
    m_cube_shape = physics.make_shape(desc);

    desc.type = physics::collision_shape_type::BOX;
    desc.box.length = 10.0f;
    desc.box.height = 0.05f;
    desc.box.width = 10.0f;
    m_plane_shape = physics.make_shape(desc);

    m_pipeline = std::make_unique<graphics::standard_pipeline>();

    graphics::geometry_data cube_data = graphics::geometry::box(1.0f, 1.0f, 1.0f);
    m_cube_vertex_buffers.push_back(
        graphics.make_vertex_buffer(cube_data.position.data(), cube_data.position.size()));
    m_cube_vertex_buffers.push_back(
        graphics.make_vertex_buffer(cube_data.normal.data(), cube_data.normal.size()));
    m_cube_index_buffer =
        graphics.make_index_buffer(cube_data.indices.data(), cube_data.indices.size());
    m_cube_material = graphics.make_pipeline_parameter("standard_material");
    m_cube_material->set(0, math::float3{1.0f, 1.0f, 1.0f});

    // Create cube.
    {
        m_cube_1 = world.create("cube 1");
        world.add<core::link, physics::rigidbody, scene::transform, graphics::visual>(m_cube_1);

        auto& t = world.component<scene::transform>(m_cube_1);
        t.position = {1.0f, 0.0f, 0.0f};
        t.rotation = math::quaternion_plain::rotation_euler(1.0f, 1.0f, 0.5f);

        auto& r = world.component<physics::rigidbody>(m_cube_1);
        r.shape = m_cube_shape.get();
        r.mass = 1.0f;
        r.type = physics::rigidbody_type::KINEMATIC;

        auto& v = world.component<graphics::visual>(m_cube_1);
        m_cube_object.emplace_back(graphics.make_pipeline_parameter("ash_object"));
        v.object = m_cube_object.back().get();
        for (auto& vertex_buffer : m_cube_vertex_buffers)
            v.vertex_buffers.push_back(vertex_buffer.get());
        v.index_buffer = m_cube_index_buffer.get();

        graphics::submesh submesh = {};
        submesh.vertex_base = 0;
        submesh.index_start = 0;
        submesh.index_end = cube_data.indices.size();
        v.submeshes.push_back(submesh);

        graphics::material material = {};
        material.pipeline = m_pipeline.get();
        material.parameters = {v.object, m_cube_material.get()};
        v.materials.push_back(material);

        relation.link(m_cube_1, scene.root());
    }

    // Cube 2.
    {
        m_cube_2 = world.create("cube 2");
        world.add<core::link, physics::rigidbody, scene::transform, graphics::visual>(m_cube_2);

        auto& t = world.component<scene::transform>(m_cube_2);
        t.position = {-1.0f, 0.0f, 0.0f};
        t.rotation = math::quaternion_plain::rotation_euler(1.0f, 1.0f, 0.5f);

        auto& r = world.component<physics::rigidbody>(m_cube_2);
        r.shape = m_cube_shape.get();
        r.mass = 1.0f;

        auto& v = world.component<graphics::visual>(m_cube_2);
        m_cube_object.emplace_back(graphics.make_pipeline_parameter("ash_object"));
        v.object = m_cube_object.back().get();
        for (auto& vertex_buffer : m_cube_vertex_buffers)
            v.vertex_buffers.push_back(vertex_buffer.get());
        v.index_buffer = m_cube_index_buffer.get();

        graphics::submesh submesh = {};
        submesh.vertex_base = 0;
        submesh.index_start = 0;
        submesh.index_end = cube_data.indices.size();
        v.submeshes.push_back(submesh);

        graphics::material material = {};
        material.pipeline = m_pipeline.get();
        material.parameters = {v.object, m_cube_material.get()};
        v.materials.push_back(material);

        relation.link(m_cube_2, m_cube_1);
    }

    // Create plane.
    {
        m_plane = world.create("plane");
        world.add<core::link, physics::rigidbody, scene::transform>(m_plane);

        auto& pt = world.component<scene::transform>(m_plane);
        pt.position = {0.0f, -3.0f, 0.0f};

        auto& pr = world.component<physics::rigidbody>(m_plane);
        pr.shape = m_plane_shape.get();
        pr.mass = 0.0f;

        relation.link(m_plane, scene.root());
    }

    // Initialize camera.
    m_camera = world.create("main camera");
    world.add<core::link, graphics::camera, scene::transform>(m_camera);
    auto extent = graphics.render_extent();
    resize_camera(extent.width, extent.height);
    graphics.game_camera(m_camera);

    relation.link(m_camera, scene.root());

    // Initialize task.
    auto& task = system<task::task_manager>();
    auto game_logic = task.schedule("sample task", []() {
        auto& physics = system<physics::physics>();
        physics.simulation();
    });
    game_logic->add_dependency(*task.find(task::TASK_GAME_LOGIC_START));
    task.find(task::TASK_GAME_LOGIC_END)->add_dependency(*game_logic);

    auto& event = system<core::event>();
    event.subscribe<graphics::event_render_extent_change>(
        "editor sample",
        [this](std::uint32_t width, std::uint32_t height) { resize_camera(width, height); });

    return true;
}

void test_module::resize_camera(std::uint32_t width, std::uint32_t height)
{
    auto& world = system<ecs::world>();
    auto& graphics = system<graphics::graphics>();

    auto& camera = world.component<graphics::camera>(m_camera);

    graphics::render_target_info render_target_info = {};
    render_target_info.width = width;
    render_target_info.height = height;
    render_target_info.format = graphics.back_buffer_format();
    render_target_info.samples = 4;
    m_render_target = graphics.make_render_target(render_target_info);
    camera.render_target(m_render_target.get());

    graphics::depth_stencil_buffer_info depth_stencil_buffer_info = {};
    depth_stencil_buffer_info.width = width;
    depth_stencil_buffer_info.height = height;
    depth_stencil_buffer_info.format = graphics::resource_format::D24_UNORM_S8_UINT;
    depth_stencil_buffer_info.samples = 4;
    m_depth_stencil_buffer = graphics.make_depth_stencil_buffer(depth_stencil_buffer_info);
    camera.depth_stencil_buffer(m_depth_stencil_buffer.get());
}
} // namespace ash::editor