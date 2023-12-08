#include "components/rigidbody.hpp"
#include "physics/physics_world.hpp"

namespace violet
{
float4x4 rigidbody_reflector::reflect(
    const float4x4& rigidbody_world,
    const float4x4& transform_world)
{
    return rigidbody_world;
}

rigidbody::rigidbody(physics_context* context) noexcept
    : m_collision_group(1),
      m_collision_mask(0xFFFFFFFF),
      m_rigidbody(nullptr),
      m_world(nullptr),
      m_context(context)
{
    m_offset = m_offset_inverse = matrix::identity();

    m_desc.type = PEI_RIGIDBODY_TYPE_DYNAMIC;
    m_desc.shape = nullptr;
    m_desc.mass = 0.0f;
    m_desc.linear_damping = 0.0f;
    m_desc.angular_damping = 0.0f;
    m_desc.restitution = 0.0f;
    m_desc.friction = 0.0f;
    m_desc.initial_transform = matrix::identity();

    m_reflector = std::make_unique<rigidbody_reflector>();
}

rigidbody::rigidbody(rigidbody&& other) noexcept
{
    m_collision_group = other.m_collision_group;
    m_collision_mask = other.m_collision_mask;

    m_offset = other.m_offset;
    m_offset_inverse = other.m_offset_inverse;

    m_desc = other.m_desc;
    m_rigidbody = std::move(other.m_rigidbody);
    m_joints = std::move(other.m_joints);
    m_slave_joints = other.m_slave_joints;
    m_world = other.m_world;
    m_context = other.m_context;
    m_reflector = std::move(other.m_reflector);

    other.m_world = nullptr;
    other.m_context = nullptr;
    other.m_joints.clear();
    other.m_slave_joints.clear();

    for (auto& joint : m_joints)
        joint->m_source = this;
}

rigidbody::~rigidbody()
{
    std::vector<joint*> joints;
    for (auto& joint : m_joints)
        joints.push_back(joint.get());

    for (auto& joint : joints)
        remove_joint(joint);

    auto slave_joints = m_slave_joints;
    for (auto joint : slave_joints)
        joint->m_source->remove_joint(joint);

    if (m_world)
        m_world->remove(m_rigidbody.get());
}

void rigidbody::set_type(pei_rigidbody_type type)
{
    m_desc.type = type;
}

void rigidbody::set_shape(pei_collision_shape* shape)
{
    m_desc.shape = shape;
    if (m_rigidbody)
        m_rigidbody->set_shape(shape);
}

void rigidbody::set_mass(float mass) noexcept
{
    m_desc.mass = mass;
    if (m_rigidbody)
        m_rigidbody->set_mass(mass);
}

void rigidbody::set_damping(float linear_damping, float angular_damping)
{
    m_desc.linear_damping = linear_damping;
    m_desc.angular_damping = angular_damping;
    if (m_rigidbody)
        m_rigidbody->set_damping(linear_damping, angular_damping);
}

void rigidbody::set_restitution(float restitution)
{
    m_desc.restitution = restitution;
    if (m_rigidbody)
        m_rigidbody->set_restitution(restitution);
}

void rigidbody::set_friction(float friction)
{
    m_desc.friction = friction;
    if (m_rigidbody)
        m_rigidbody->set_restitution(friction);
}

void rigidbody::set_transform(const float4x4& transform)
{
    if (m_rigidbody)
        m_rigidbody->set_transform(transform);
    else
        m_desc.initial_transform = transform;
}

const float4x4& rigidbody::get_transform() const
{
    return m_rigidbody->get_transform();
}

void rigidbody::set_offset(const float4x4& offset) noexcept
{
    m_offset = offset;
    m_offset_inverse = matrix::inverse(offset);
}

void rigidbody::set_activation_state(pei_rigidbody_activation_state activation_state)
{
    if (m_rigidbody)
        m_rigidbody->set_activation_state(activation_state);
    else
        m_desc.activation_state = activation_state;
}

void rigidbody::clear_forces()
{
    if (m_rigidbody)
        m_rigidbody->clear_forces();
}

joint* rigidbody::add_joint(
    component_ptr<rigidbody> target,
    const float3& source_position,
    const float4& source_rotation,
    const float3& target_position,
    const float4& target_rotation)
{
    m_joints.push_back(std::make_unique<joint>(
        this,
        target,
        source_position,
        source_rotation,
        target_position,
        target_rotation,
        m_context));

    joint* result = m_joints.back().get();
    target->m_slave_joints.push_back(result);
    return result;
}

void rigidbody::remove_joint(joint* joint)
{
    auto& target_joints = joint->m_target->m_slave_joints;
    for (std::size_t i = 0; i < target_joints.size(); ++i)
    {
        if (target_joints[i] == joint)
        {
            std::swap(target_joints[i], target_joints.back());
            target_joints.pop_back();
            break;
        }
    }

    for (std::size_t i = 0; i < m_joints.size(); ++i)
    {
        if (m_joints[i].get() == joint)
        {
            std::swap(m_joints[i], m_joints.back());
            m_joints.pop_back();
            break;
        }
    }
}

void rigidbody::set_updated_flag(bool flag)
{
    m_rigidbody->set_updated_flag(flag);
}

bool rigidbody::get_updated_flag() const
{
    return m_rigidbody->get_updated_flag();
}

pei_rigidbody* rigidbody::get_rigidbody()
{
    if (m_rigidbody == nullptr)
        m_rigidbody = m_context->create_rigidbody(m_desc);

    return m_rigidbody.get();
}

std::vector<pei_joint*> rigidbody::get_joints()
{
    std::vector<pei_joint*> joints;
    for (auto& joint : m_joints)
        joints.push_back(joint->get_joint());

    return joints;
}

void rigidbody::set_world(pei_world* world)
{
    m_world = world;
}

rigidbody& rigidbody::operator=(rigidbody&& other) noexcept
{
    if (this == &other)
        return *this;

    m_collision_group = other.m_collision_group;
    m_collision_mask = other.m_collision_mask;

    m_offset = other.m_offset;
    m_offset_inverse = other.m_offset_inverse;

    m_desc = other.m_desc;
    m_rigidbody = std::move(other.m_rigidbody);
    m_joints = std::move(other.m_joints);
    m_slave_joints = other.m_slave_joints;
    m_world = other.m_world;
    m_context = other.m_context;
    m_reflector = std::move(other.m_reflector);

    other.m_rigidbody = nullptr;
    other.m_world = nullptr;
    other.m_context = nullptr;
    other.m_joints.clear();
    other.m_slave_joints.clear();

    for (auto& joint : m_joints)
        joint->m_source = this;

    return *this;
}

joint::joint(
    rigidbody* source,
    component_ptr<rigidbody> target,
    const float3& source_position,
    const float4& source_rotation,
    const float3& target_position,
    const float4& target_rotation,
    physics_context* context)
{
    m_source = source;
    m_target = target;

    pei_joint_desc desc = {};
    desc.source = source->get_rigidbody();
    desc.source_position = source_position;
    desc.source_rotation = source_rotation;
    desc.target = target->get_rigidbody();
    desc.target_position = target_position;
    desc.target_rotation = target_rotation;

    m_joint = context->create_joint(desc);
}

joint::~joint()
{
}

void joint::set_linear(const float3& min, const float3& max)
{
    if (m_joint)
        m_joint->set_linear(min, max);
}

void joint::set_angular(const float3& min, const float3& max)
{
    if (m_joint)
        m_joint->set_angular(min, max);
}

void joint::set_spring_enable(std::size_t index, bool enable)
{
    if (m_joint)
        m_joint->set_spring_enable(index, enable);
}

void joint::set_stiffness(std::size_t index, float stiffness)
{
    if (m_joint)
        m_joint->set_stiffness(index, stiffness);
}

void joint::set_damping(std::size_t index, float damping)
{
    if (m_joint)
        m_joint->set_damping(index, damping);
}
} // namespace violet