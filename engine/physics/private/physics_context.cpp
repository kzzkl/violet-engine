#include "physics/physics_context.hpp"

namespace violet
{
pei_deleter::pei_deleter() : m_pei(nullptr)
{
}

pei_deleter::pei_deleter(pei_plugin* pei) : m_pei(pei)
{
}

void pei_deleter::operator()(pei_world* world)
{
    m_pei->destroy_world(world);
}

void pei_deleter::operator()(pei_collision_shape* collision_shape)
{
    m_pei->destroy_collision_shape(collision_shape);
}

void pei_deleter::operator()(pei_rigidbody* rigidbody)
{
    m_pei->destroy_rigidbody(rigidbody);
}

void pei_deleter::operator()(pei_joint* joint)
{
    m_pei->destroy_joint(joint);
}

physics_context::physics_context(pei_plugin* pei) : m_pei(pei), m_deleter(pei)
{
}

pei_ptr<pei_world> physics_context::create_world(const pei_world_desc& desc)
{
    return pei_ptr<pei_world>(m_pei->create_world(desc), m_deleter);
}

pei_ptr<pei_collision_shape> physics_context::create_collision_shape(
    const pei_collision_shape_desc& desc)
{
    return pei_ptr<pei_collision_shape>(m_pei->create_collision_shape(desc), m_deleter);
}

pei_ptr<pei_collision_shape> physics_context::create_collision_shape(
    const std::vector<pei_collision_shape*>& shapes,
    const std::vector<float4x4>& offset)
{
    return pei_ptr<pei_collision_shape>(
        m_pei->create_collision_shape(shapes.data(), offset.data(), shapes.size()),
        m_deleter);
}

pei_ptr<pei_rigidbody> physics_context::create_rigidbody(const pei_rigidbody_desc& desc)
{
    return pei_ptr<pei_rigidbody>(m_pei->create_rigidbody(desc), m_deleter);
}

pei_ptr<pei_joint> physics_context::create_joint(const pei_joint_desc& desc)
{
    return pei_ptr<pei_joint>(m_pei->create_joint(desc), m_deleter);
}
} // namespace violet