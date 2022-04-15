#include "mmd_ik.hpp"
#include "transform.hpp"

namespace ash::sample::mmd
{
mmd_ik_solver::mmd_ik_solver(ecs::world& world) : m_world(world)
{
}

void mmd_ik_solver::solve(ecs::entity entity)
{
    auto& ik_node = m_world.component<mmd_bone>(entity);
    if (!ik_node.enable_ik_solver)
        return;

    for (auto& ik_link : ik_node.links)
    {
        ik_link.prev_angle = {0.0f, 0.0f, 0.0f};
        ik_link.plane_mode_angle = 0.0f;

        auto& node = m_world.component<mmd_bone>(ik_link.node);
        node.ik_rotate = {0.0f, 0.0f, 0.0f, 1.0f};
    }

    float max_dist = std::numeric_limits<float>::max();
    for (std::uint32_t i = 0; i < ik_node.loop_count; ++i)
    {
        solve_core(entity, i);

        auto target_position = m_world.component<scene::transform>(ik_node.ik_target).position;
        auto ik_position = m_world.component<scene::transform>(entity).position;

        float dist =
            math::vector_plain::length(math::vector_plain::sub(target_position, ik_position));
        if (dist < max_dist)
        {
            max_dist = dist;
            for (auto& ik_link : ik_node.links)
                ik_link.save_ik_rotate = m_world.component<mmd_bone>(ik_link.node).ik_rotate;
        }
        else
        {
            for (auto& ik_link : ik_node.links)
            {
                auto& link_node = m_world.component<mmd_bone>(ik_link.node);
                link_node.ik_rotate = ik_link.save_ik_rotate;
                // update local
            }
        }
    }
}

void mmd_ik_solver::solve_core(ecs::entity entity, std::uint32_t index)
{
    auto& ik_node = m_world.component<mmd_bone>(entity);
    auto ik_position = math::simd::load(m_world.component<scene::transform>(entity).position);

    for (std::size_t i = 0; i < ik_node.links.size(); ++i)
    {
        auto& link = ik_node.links[i];
        if (link.node == ik_node.ik_target)
            continue;

        if (link.enable_limit)
        {
        }

        auto target_position =
            math::simd::load(m_world.component<scene::transform>(ik_node.ik_target).position);
        auto link_inverse = math::matrix_simd::inverse(
            math::simd::load(m_world.component<scene::transform>(link.node).world_matrix));

        auto link_ik_position = math::matrix_simd::mul(ik_position, link_inverse);
        auto link_target_position = math::matrix_simd::mul(target_position, link_inverse);

        auto link_ik_vec = math::vector_simd::normalize(link_ik_position);
        auto link_target_vec = math::vector_simd::normalize(link_target_position);

        auto dot = math::vector_simd::dot(link_ik_vec, link_target_vec);
        dot = math::clamp(dot, -1.0f, 1.0f);

        float angle = std::acos(dot);
        float angle_deg = math::to_degrees(angle);
        if (angle_deg < 1.0e-3f)
            continue;

        angle = math::clamp(angle, -ik_node.limit_angle, ik_node.limit_angle);
        auto cross =
            math::vector_simd::normalize(math::vector_simd::cross(link_target_vec, link_ik_vec));
        math::float4 rotate;
        math::simd::store(math::quaternion_simd::rotation_axis(cross, angle), rotate);

        auto& link_node = m_world.component<mmd_bone>(link.node);
        auto link_rotate =
            math::quaternion_plain::mul(link_node.ik_rotate, link_node.animation_rotate);
        link_rotate = math::quaternion_plain::mul(link_rotate, rotate);

        auto& link_node_transform = m_world.component<scene::transform>(link.node);
        auto link_node_animation_rotate =
            math::quaternion_plain::mul(link_node.animation_rotate, link_node_transform.rotation);
        link_node.ik_rotate = math::quaternion_plain::mul(
            link_rotate,
            math::quaternion_plain::inverse(link_node_animation_rotate));
    }
}
} // namespace ash::sample::mmd