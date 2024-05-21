#include "physics/physics_context.hpp"

namespace violet
{
phy_deleter::phy_deleter() : m_plugin(nullptr)
{
}

phy_deleter::phy_deleter(phy_plugin* plugin) : m_plugin(plugin)
{
}

void phy_deleter::operator()(phy_world* world)
{
    m_plugin->destroy_world(world);
}

void phy_deleter::operator()(phy_collision_shape* collision_shape)
{
    m_plugin->destroy_collision_shape(collision_shape);
}

void phy_deleter::operator()(phy_rigidbody* rigidbody)
{
    m_plugin->destroy_rigidbody(rigidbody);
}

void phy_deleter::operator()(phy_joint* joint)
{
    m_plugin->destroy_joint(joint);
}

physics_context::physics_context(phy_plugin* plugin) : m_plugin(plugin), m_deleter(plugin)
{
}

phy_ptr<phy_world> physics_context::create_world(const phy_world_desc& desc)
{
    return phy_ptr<phy_world>(m_plugin->create_world(desc), m_deleter);
}

phy_ptr<phy_collision_shape> physics_context::create_collision_shape(
    const phy_collision_shape_desc& desc)
{
    return phy_ptr<phy_collision_shape>(m_plugin->create_collision_shape(desc), m_deleter);
}

phy_ptr<phy_collision_shape> physics_context::create_collision_shape(
    const std::vector<phy_collision_shape*>& shapes,
    const std::vector<float4x4>& offset)
{
    return phy_ptr<phy_collision_shape>(
        m_plugin->create_collision_shape(shapes.data(), offset.data(), shapes.size()),
        m_deleter);
}

phy_ptr<phy_rigidbody> physics_context::create_rigidbody(const phy_rigidbody_desc& desc)
{
    return phy_ptr<phy_rigidbody>(m_plugin->create_rigidbody(desc), m_deleter);
}

phy_ptr<phy_joint> physics_context::create_joint(const phy_joint_desc& desc)
{
    return phy_ptr<phy_joint>(m_plugin->create_joint(desc), m_deleter);
}
} // namespace violet