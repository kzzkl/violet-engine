#include "sample.hpp"
#include "geometry.hpp"
#include "relation.hpp"
#include "scene.hpp"

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

    m_standard_pass = std::make_unique<graphics::standard_pass>();

    graphics::geometry_data cube_data = graphics::geometry::box(1.0f, 1.0f, 1.0f);
    m_cube_vertex_buffer =
        graphics.make_vertex_buffer(cube_data.vertices.data(), cube_data.vertices.size());
    m_cube_index_buffer =
        graphics.make_index_buffer(cube_data.indices.data(), cube_data.indices.size());
    m_cube_material = graphics.make_pipeline_parameter("standard_material");
    m_cube_material->set(0, math::float4{1.0f, 1.0f, 1.0f, 1.0f});

    // Create cube.
    {
        m_cube_1 = world.create();
        world.add<core::link, physics::rigidbody, scene::transform, graphics::visual>(m_cube_1);

        auto& t = world.component<scene::transform>(m_cube_1);
        t.position = {1.0f, 0.0f, 0.0f};
        t.rotation = math::quaternion_plain::rotation_euler(1.0f, 1.0f, 0.5f);

        auto& r = world.component<physics::rigidbody>(m_cube_1);
        r.shape = m_cube_shape.get();
        r.mass = 1.0f;
        r.type = physics::rigidbody_type::KINEMATIC;
        r.relation = m_cube_1;

        auto& v = world.component<graphics::visual>(m_cube_1);
        m_cube_object.emplace_back(graphics.make_pipeline_parameter("ash_object"));
        v.object = m_cube_object.back().get();

        graphics::render_unit submesh;
        submesh.vertex_buffer = m_cube_vertex_buffer.get();
        submesh.index_buffer = m_cube_index_buffer.get();
        submesh.index_start = 0;
        submesh.index_end = cube_data.indices.size();
        submesh.render_pass = m_standard_pass.get();
        submesh.parameters = {v.object, m_cube_material.get()};
        v.submesh.push_back(submesh);

        relation.link(m_cube_1, scene.root());
    }

    // Cube 2.
    {
        m_cube_2 = world.create();
        world.add<core::link, physics::rigidbody, scene::transform, graphics::visual>(m_cube_2);

        auto& t = world.component<scene::transform>(m_cube_2);
        t.position = {-1.0f, 0.0f, 0.0f};
        t.rotation = math::quaternion_plain::rotation_euler(1.0f, 1.0f, 0.5f);

        auto& r = world.component<physics::rigidbody>(m_cube_2);
        r.shape = m_cube_shape.get();
        r.mass = 1.0f;
        r.relation = m_cube_2;

        auto& v = world.component<graphics::visual>(m_cube_2);
        m_cube_object.emplace_back(graphics.make_pipeline_parameter("ash_object"));
        v.object = m_cube_object.back().get();

        graphics::render_unit submesh;
        submesh.vertex_buffer = m_cube_vertex_buffer.get();
        submesh.index_buffer = m_cube_index_buffer.get();
        submesh.index_start = 0;
        submesh.index_end = cube_data.indices.size();
        submesh.render_pass = m_standard_pass.get();
        submesh.parameters = {v.object, m_cube_material.get()};
        v.submesh.push_back(submesh);

        relation.link(m_cube_2, m_cube_1);
    }

    // Create plane.
    {
        m_plane = world.create();
        world.add<core::link, physics::rigidbody, scene::transform>(m_plane);

        auto& pt = world.component<scene::transform>(m_plane);
        pt.position = {0.0f, -3.0f, 0.0f};

        auto& pr = world.component<physics::rigidbody>(m_plane);
        pr.shape = m_plane_shape.get();
        pr.mass = 0.0f;
        pr.relation = m_plane;

        relation.link(m_plane, scene.root());
    }

    return true;
}
} // namespace ash::editor