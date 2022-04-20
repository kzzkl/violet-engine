#pragma once

#include "rigidbody.hpp"

namespace ash::physics
{
struct joint
{
    ecs::entity relation_a;
    ecs::entity relation_b;

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

/*class joint
{
public:
    joint() = default;
    joint(joint&&) = default;
    joint(const joint&) = delete;

    std::size_t add_unit();

    void rigidbody(std::size_t index, ecs::entity entity);

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
        ecs::entity entity;

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
};*/
} // namespace ash::physics