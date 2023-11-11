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

rigidbody::rigidbody(pei_plugin* pei) noexcept
    : m_collision_group(1),
      m_collision_mask(0xFFFFFFFF),
      m_rigidbody(nullptr),
      m_world(nullptr),
      m_pei(pei)
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
    m_rigidbody = other.m_rigidbody;
    m_joints = std::move(other.m_joints);
    m_world = other.m_world;
    m_pei = other.m_pei;
    m_reflector = std::move(other.m_reflector);

    other.m_rigidbody = nullptr;
    other.m_world = nullptr;
    other.m_pei = nullptr;
}

rigidbody::~rigidbody()
{
    for (auto& joint : m_joints)
    {
        if (joint->m_joint)
        {
            if (m_world)
                m_world->remove(joint->m_joint);
            m_pei->destroy_joint(joint->m_joint);
        }
    }

    if (m_world)
        m_world->remove(m_rigidbody);

    if (m_rigidbody)
        m_pei->destroy_rigidbody(m_rigidbody);
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

joint* rigidbody::add_joint(const float3& position, const float4& rotation)
{
    m_joints.push_back(std::make_unique<joint>());
    m_joints.back()->m_desc.source_position = position;
    m_joints.back()->m_desc.source_rotation = rotation;
    return m_joints.back().get();
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
        m_rigidbody = m_pei->create_rigidbody(m_desc);

    return m_rigidbody;
}

std::vector<pei_joint*> rigidbody::get_joints()
{
    std::vector<pei_joint*> joints;
    for (auto& joint : m_joints)
    {
        if (joint->m_joint == nullptr)
        {
            joint->m_desc.source = get_rigidbody();
            joint->m_desc.target = joint->m_target->get_rigidbody();
            joint->m_joint = m_pei->create_joint(joint->m_desc);
        }

        joints.push_back(joint->m_joint);
    }

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

    if (m_rigidbody)
        m_pei->destroy_rigidbody(m_rigidbody);

    m_collision_group = other.m_collision_group;
    m_collision_mask = other.m_collision_mask;

    m_offset = other.m_offset;
    m_offset_inverse = other.m_offset_inverse;

    m_desc = other.m_desc;
    m_rigidbody = other.m_rigidbody;
    m_joints = std::move(other.m_joints);
    m_world = other.m_world;
    m_pei = other.m_pei;
    m_reflector = std::move(other.m_reflector);

    other.m_rigidbody = nullptr;
    other.m_world = nullptr;
    other.m_pei = nullptr;

    return *this;
}

joint::joint() : m_desc{}, m_joint(nullptr)
{
    m_desc.source_rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    m_desc.target_rotation = {0.0f, 0.0f, 0.0f, 1.0f};
}

void joint::set_target(
    component_ptr<rigidbody> target,
    const float3& position,
    const float4& rotation)
{
    m_target = target;
    m_desc.target_position = position;
    m_desc.target_rotation = rotation;
}

void joint::set_linear(const float3& min, const float3& max)
{
    m_desc.min_linear = min;
    m_desc.max_linear = max;
    if (m_joint)
        m_joint->set_linear(min, max);
}

void joint::set_angular(const float3& min, const float3& max)
{
    m_desc.min_angular = min;
    m_desc.max_angular = max;
    if (m_joint)
        m_joint->set_angular(min, max);
}

void joint::set_spring_enable(std::size_t index, bool enable)
{
    m_desc.spring_enable[index] = enable;
    if (m_joint)
        m_joint->set_spring_enable(index, enable);
}

void joint::set_stiffness(std::size_t index, float stiffness)
{
    m_desc.stiffness[index] = stiffness;
    if (m_joint)
        m_joint->set_stiffness(index, stiffness);
}

void joint::set_damping(std::size_t index, float damping)
{
    m_desc.damping[index] = damping;
    if (m_joint)
        m_joint->set_damping(index, damping);
}
} // namespace violet