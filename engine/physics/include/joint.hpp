#pragma once

#include "rigidbody.hpp"

namespace ash::physics
{
class PHYSICS_API joint
{
public:
    using rigidbody_handle = ash::ecs::component_handle<rigidbody>;

public:
    joint() = default;
    joint(joint&&) = default;
    joint(const joint&) = delete;

    std::size_t add_unit();

    void rigidbody(std::size_t index, rigidbody_handle rigidbody);

    void location(std::size_t index, const math::float3& location);
    void rotation(std::size_t index, const math::float4& rotation);

    void min_linear(std::size_t index, const math::float3& min_linear);
    void max_linear(std::size_t index, const math::float3& max_linear);

    void min_angular(std::size_t index, const math::float3& min_angular);
    void max_angular(std::size_t index, const math::float3& max_angular);

    void spring_translate_factor(std::size_t index, const math::float3& spring_translate_factor);
    void spring_rotate_factor(std::size_t index, const math::float3& spring_rotate_factor);

    auto begin() { return m_units.begin(); }
    auto end() { return m_units.end(); }

    joint& operator=(joint&&) = default;
    joint& operator=(const joint&) = delete;

private:
    struct unit
    {
        rigidbody_handle rigidbody;

        math::float3 location;
        math::float4 rotation;

        math::float3 min_linear;
        math::float3 max_linear;

        math::float3 min_angular;
        math::float3 max_angular;

        math::float3 spring_translate_factor;
        math::float3 spring_rotate_factor;

        std::unique_ptr<joint_interface> interface;
    };

    std::vector<unit> m_units;
};
} // namespace ash::physics

namespace ash::ecs
{
template <>
struct component_trait<ash::physics::joint>
{
    static constexpr std::size_t id = ash::uuid("4a535d0b-d271-4875-a32d-fe6394d43368").hash();
};
} // namespace ash::ecs