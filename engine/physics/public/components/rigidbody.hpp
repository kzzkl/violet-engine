#pragma once

#include "physics/physics_interface.hpp"

namespace violet
{
class rigidbody
{
public:
    rigidbody();
    rigidbody(const rigidbody&) = delete;
    rigidbody(rigidbody&& other);
    ~rigidbody();

    void set_type(pei_rigidbody_type type);
    void set_shape(pei_collision_shape* shape);
    void set_mass(float mass) noexcept;
    void set_damping(float linear_damping, float angular_damping);
    void set_restitution(float restitution);
    void set_friction(float friction);

    void set_collision_group(std::size_t group) { m_collision_group = group; }
    std::size_t get_collision_group() const noexcept { return m_collision_group; }

    void set_collision_mask(std::size_t mask) { m_collision_mask = mask; }
    std::size_t get_collision_mask() const noexcept { return m_collision_mask; }

    void set_transform(const float4x4& transform);
    const float4x4& get_transform() const;

    void set_updated_flag(bool flag);
    bool get_updated_flag() const;

    rigidbody& operator=(const rigidbody&) = delete;
    rigidbody& operator=(rigidbody&& other);

private:
    friend class physics_world;

    std::uint32_t m_collision_group;
    std::uint32_t m_collision_mask;

    pei_rigidbody_desc m_desc;
    pei_rigidbody* m_rigidbody;
    pei_plugin* m_pei;
};
} // namespace violet