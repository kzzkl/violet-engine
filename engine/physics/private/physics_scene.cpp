#include "physics/physics_scene.hpp"

namespace violet
{
physics_scene::physics_scene(const vec3f& gravity, phy_debug_draw* debug, physics_context* context)
{
    phy_world_desc desc = {
        .gravity = gravity,
        .debug_draw = debug,
    };
    m_world = context->create_world(desc);
}

physics_scene::~physics_scene() {}

void physics_scene::add_rigidbody(phy_rigidbody* rigidbody)
{
    m_world->add(rigidbody);
}

void physics_scene::remove_rigidbody(phy_rigidbody* rigidbody)
{
    m_world->remove(rigidbody);
}

void physics_scene::add_joint(phy_joint* joint)
{
    m_world->add(joint);
}

void physics_scene::remove_joint(phy_joint* joint)
{
    m_world->remove(joint);
}

/*oid physics_scene::add(actor* actor)
{
    auto added_rigidbody = actor->get<rigidbody>();
    auto added_transform = actor->get<transform>();

    matrix4 added_offset = math::load(added_rigidbody->get_offset());
    matrix4 added_world_matrix = math::load(added_transform->get_world_matrix());
    mat4f rigidbody_transform;
    math::store(matrix::mul(added_offset, added_world_matrix), rigidbody_transform);

    added_rigidbody->set_transform(rigidbody_transform);
    added_rigidbody->set_world(m_world.get());

    m_world->add(
        added_rigidbody->get_rigidbody(),
        added_rigidbody->get_collision_group(),
        added_rigidbody->get_collision_mask());

    for (phy_joint* joint : added_rigidbody->get_joints())
        m_world->add(joint);
}*/

void physics_scene::simulation(float time_step)
{
    m_world->simulation(time_step);
}
} // namespace violet