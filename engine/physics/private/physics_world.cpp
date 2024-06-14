#include "physics/physics_world.hpp"
#include "components/rigidbody.hpp"
#include "components/transform.hpp"

namespace violet
{
physics_world::physics_world(const float3& gravity, phy_debug_draw* debug, physics_context* context)
{
    phy_world_desc desc = {};
    desc.gravity = gravity;
    desc.debug_draw = debug;
    m_world = context->create_world(desc);
}

physics_world::~physics_world()
{
}

void physics_world::add(actor* actor)
{
    auto added_rigidbody = actor->get<rigidbody>();
    auto added_transform = actor->get<transform>();

    matrix4 added_offset = math::load(added_rigidbody->get_offset());
    matrix4 added_world_matrix = math::load(added_transform->get_world_matrix());
    float4x4 rigidbody_transform;
    math::store(matrix::mul(added_offset, added_world_matrix), rigidbody_transform);

    added_rigidbody->set_transform(rigidbody_transform);
    added_rigidbody->set_world(m_world.get());

    m_world->add(
        added_rigidbody->get_rigidbody(),
        added_rigidbody->get_collision_group(),
        added_rigidbody->get_collision_mask());

    for (phy_joint* joint : added_rigidbody->get_joints())
        m_world->add(joint);
}

void physics_world::simulation(float time_step)
{
    m_world->simulation(time_step);
}
} // namespace violet