#include "components/rigidbody.hpp"

namespace violet
{
rigidbody::rigidbody()
    : m_collision_group(0),
      m_collision_mask(0xFFFFFFFF),
      m_rigidbody(nullptr),
      m_pei(nullptr)
{
    m_desc.type = PEI_RIGIDBODY_TYPE_DYNAMIC;
    m_desc.shape = nullptr;
    m_desc.mass = 0.0f;
    m_desc.linear_damping = 0.0f;
    m_desc.angular_damping = 0.0f;
    m_desc.restitution = 0.0f;
    m_desc.friction = 0.0f;
    m_desc.initial_transform = matrix::identity();
}

rigidbody::rigidbody(rigidbody&& other) : rigidbody()
{
    *this = std::move(other);
}

rigidbody::~rigidbody()
{
    if (m_pei && m_rigidbody)
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

void rigidbody::set_updated_flag(bool flag)
{
    m_rigidbody->set_updated_flag(flag);
}

bool rigidbody::get_updated_flag() const
{
    return m_rigidbody->get_updated_flag();
}

rigidbody& rigidbody::operator=(rigidbody&& other)
{
    if (this == &other)
        return *this;

    if (m_rigidbody)
        m_pei->destroy_rigidbody(m_rigidbody);

    m_collision_group = other.m_collision_group;
    m_collision_mask = other.m_collision_mask;

    m_desc = other.m_desc;
    m_rigidbody = other.m_rigidbody;
    m_pei = other.m_pei;

    other.m_rigidbody = nullptr;
    other.m_pei = nullptr;

    return *this;
}
} // namespace violet