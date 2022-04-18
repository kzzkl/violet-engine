#include "mmd_animation.hpp"
#include "scene.hpp"

namespace ash::sample::mmd
{
namespace
{
float EvalX(const math::float4& cp, float t)
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float it = 1.0f - t;
    const float it2 = it * it;
    const float it3 = it2 * it;
    const float x[4] = {0, cp[0], cp[2], 1};

    return t3 * x[3] + 3 * t2 * it * x[2] + 3 * t * it2 * x[1] + it3 * x[0];
}

float EvalY(const math::float4& cp, float t)
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float it = 1.0f - t;
    const float it2 = it * it;
    const float it3 = it2 * it;
    const float y[4] = {0, cp[1], cp[3], 1};

    return t3 * y[3] + 3 * t2 * it * y[2] + 3 * t * it2 * y[1] + it3 * y[0];
}

float FindBezierX(const math::float4& cp, float time)
{
    const float e = 0.00001f;
    float start = 0.0f;
    float stop = 1.0f;
    float t = 0.5f;
    float x = EvalX(cp, t);
    while (std::abs(time - x) > e)
    {
        if (time < x)
        {
            stop = t;
        }
        else
        {
            start = t;
        }
        t = (stop + start) * 0.5f;
        x = EvalX(cp, t);
    }

    return t;
}
} // namespace

bool mmd_animation::initialize(const dictionary& config)
{
    auto& world = system<ash::ecs::world>();
    world.register_component<mmd_node_animation>();
    world.register_component<mmd_ik_solver>();
    world.register_component<mmd_ik_link>();

    m_view = world.make_view<mmd_skeleton>();
    m_node_view = world.make_view<mmd_node, mmd_node_animation>();
    m_ik_view = world.make_view<mmd_node, mmd_ik_solver>();
    m_transform_view = world.make_view<mmd_node, scene::transform>();

    return true;
}

void mmd_animation::evaluate(float t, float weight)
{
    std::map<std::string, std::pair<mmd_node*, mmd_node_animation*>> m;
    m_node_view->each([=, &m](mmd_node& node, mmd_node_animation& node_animation) {
        evaluate_node(node, node_animation, t, weight);
    });

    m_ik_view->each([=](mmd_node& node, mmd_ik_solver& ik) { evaluate_ik(node, ik, t, weight); });
}

void mmd_animation::evaluate_node(
    mmd_node& node,
    mmd_node_animation& node_animation,
    float t,
    float weight)
{
    if (node_animation.keys.empty())
    {
        node_animation.animation_translate = {0.0f, 0.0f, 0.0f};
        node_animation.animation_rotate = {0.0f, 0.0f, 0.0f, 1.0f};
        return;
    }

    auto bound =
        bound_key(node_animation.keys, static_cast<std::int32_t>(t), node_animation.offset);

    math::float3 translate;
    math::float4 rotate;
    if (bound == node_animation.keys.end())
    {
        translate = node_animation.keys.back().translate;
        rotate = node_animation.keys.back().rotate;
    }
    else
    {
        translate = bound->translate;
        rotate = bound->rotate;
        if (bound != node_animation.keys.begin())
        {
            const auto& key0 = *(bound - 1);
            const auto& key1 = *bound;

            float timeRange = float(key1.frame - key0.frame);
            float time = (t - float(key0.frame)) / timeRange;
            float tx_x = FindBezierX(key0.tx_bezier, time);
            float ty_x = FindBezierX(key0.ty_bezier, time);
            float tz_x = FindBezierX(key0.tz_bezier, time);
            float rot_x = FindBezierX(key0.r_bezier, time);
            float tx_y = EvalY(key0.tx_bezier, tx_x);
            float ty_y = EvalY(key0.ty_bezier, ty_x);
            float tz_y = EvalY(key0.tz_bezier, tz_x);
            float rot_y = EvalY(key0.r_bezier, rot_x);

            translate = math::vector_plain::mix(
                key0.translate,
                key1.translate,
                math::float3{tx_y, ty_y, tz_y});
            rotate = math::quaternion_plain::slerp(key0.rotate, key1.rotate, rot_y);

            node_animation.offset = std::distance(node_animation.keys.cbegin(), bound);
            //log::debug("{} {} {}", node.name, node_animation.offset, node_animation.keys.size());
        }
    }

    if (weight == 1.0f)
    {
        node_animation.animation_translate = translate;
        node_animation.animation_rotate = rotate;
    }
    else
    {
        node_animation.animation_rotate =
            math::quaternion_plain::slerp(node_animation.base_animation_rotate, rotate, weight);
        node_animation.animation_translate =
            math::vector_plain::mix(node_animation.base_animation_translate, translate, weight);
    }
}

void mmd_animation::update(bool after_physics)
{
    auto& world = system<ecs::world>();
    m_view->each([&](ecs::entity entity, mmd_skeleton& skeleton) {
        update_local(skeleton, after_physics);
        update_world(skeleton, after_physics);
    });

    m_view->each([&](ecs::entity entity, mmd_skeleton& skeleton) {
        for (auto& node_entity : skeleton.sorted_nodes)
        {
            auto& node = world.component<mmd_node>(node_entity);
            if (after_physics == node.deform_after_physics &&
                node.inherit_node != ecs::INVALID_ENTITY)
            {
                auto& transform = world.component<scene::transform>(node_entity);
                auto& animation = world.component<mmd_node_animation>(node_entity);

                update_inherit(node, animation, transform);
                update_transform(skeleton, node.index);
            }

            if (world.has_component<mmd_ik_solver>(node_entity))
            {
                auto& solver = world.component<mmd_ik_solver>(node_entity);
                if (solver.enable)
                    update_ik(skeleton, node, solver);
                update_transform(skeleton, node.index);
            }
        }
    });
    m_view->each([&](ecs::entity entity, mmd_skeleton& skeleton) {
        update_local(skeleton, after_physics);
        update_world(skeleton, after_physics);
    });
}

void mmd_animation::evaluate_ik(mmd_node& node, mmd_ik_solver& ik, float t, float weight)
{
    if (ik.keys.empty())
    {
        ik.enable = true;
        return;
    }

    auto bound = bound_key(ik.keys, static_cast<std::int32_t>(t), ik.offset);
    bool enable = true;
    if (bound == ik.keys.end())
    {
        enable = ik.keys.back().enable;
    }
    else
    {
        // TODO
        enable = ik.keys.front().enable;
        if (bound != ik.keys.begin())
        {
            const auto& key = *(bound - 1);
            enable = key.enable;

            ik.offset = std::distance(ik.keys.cbegin(), bound);
        }
    }

    if (weight == 1.0f)
    {
        ik.enable = enable;
    }
    else
    {
        if (weight < 1.0f)
            ik.enable = true;
        else
            ik.enable = enable;
    }
}

void mmd_animation::update_local(mmd_skeleton& skeleton, bool after_physics)
{
    auto& world = system<ecs::world>();

    for (auto& node_entity : skeleton.sorted_nodes)
    {
        auto& node = world.component<mmd_node>(node_entity);
        if (after_physics != node.deform_after_physics)
            continue;

        auto& transform = world.component<scene::transform>(node_entity);
        auto& animation = world.component<mmd_node_animation>(node_entity);

        math::float3 translate =
            math::vector_plain::add(animation.animation_translate, transform.position);
        if (node.is_inherit_translation)
            translate = math::vector_plain::add(translate, node.inherit_translate);

        math::float4 rotate =
            math::quaternion_plain::mul(animation.animation_rotate, transform.rotation);
        if (world.has_component<mmd_ik_link>(node_entity))
            rotate = math::quaternion_plain::mul(
                world.component<mmd_ik_link>(node_entity).ik_rotate,
                rotate);
        if (node.is_inherit_rotation)
            rotate = math::quaternion_plain::mul(rotate, node.inherit_rotate);

        skeleton.local[node.index] =
            math::matrix_plain::affine_transform(transform.scaling, rotate, translate);
    }
}

void mmd_animation::update_world(mmd_skeleton& skeleton, bool after_physics)
{
    auto& world = system<ecs::world>();

    for (auto& node_entity : skeleton.sorted_nodes)
    {
        auto& node = world.component<mmd_node>(node_entity);
        if (after_physics != node.deform_after_physics)
            continue;

        auto& transform = world.component<scene::transform>(node_entity);

        if (world.has_component<mmd_node>(transform.parent))
        {
            auto& parent = world.component<mmd_node>(transform.parent);
            skeleton.world[node.index] =
                math::matrix_plain::mul(skeleton.local[node.index], skeleton.world[parent.index]);
        }
        else
        {
            auto& parent = world.component<scene::transform>(transform.parent);
            skeleton.world[node.index] =
                math::matrix_plain::mul(skeleton.local[node.index], parent.world_matrix);
        }
    }
}

void mmd_animation::update_transform(mmd_skeleton& skeleton, std::size_t index)
{
    auto& world = system<ecs::world>();
    auto& root = skeleton.nodes[index];

    // Update local.
    {
        auto& node = world.component<mmd_node>(root);
        auto& transform = world.component<scene::transform>(root);
        auto& animation = world.component<mmd_node_animation>(root);

        math::float3 translate =
            math::vector_plain::add(animation.animation_translate, transform.position);
        if (node.is_inherit_translation)
            translate = math::vector_plain::add(translate, node.inherit_translate);

        math::float4 rotate =
            math::quaternion_plain::mul(animation.animation_rotate, transform.rotation);
        if (world.has_component<mmd_ik_link>(root))
            rotate =
                math::quaternion_plain::mul(world.component<mmd_ik_link>(root).ik_rotate, rotate);
        if (node.is_inherit_rotation)
            rotate = math::quaternion_plain::mul(rotate, node.inherit_rotate);

        skeleton.local[node.index] =
            math::matrix_plain::affine_transform(transform.scaling, rotate, translate);
    }

    // Update world.
    {
        std::queue<ecs::entity> bfs;
        bfs.push(root);

        while (!bfs.empty())
        {
            ecs::entity node_entity = bfs.front();
            bfs.pop();

            auto& node = world.component<mmd_node>(node_entity);
            auto& transform = world.component<scene::transform>(node_entity);

            if (world.has_component<mmd_node>(transform.parent))
            {
                auto& parent = world.component<mmd_node>(transform.parent);
                skeleton.world[node.index] = math::matrix_plain::mul(
                    skeleton.local[node.index],
                    skeleton.world[parent.index]);
            }
            else
            {
                auto& parent = world.component<scene::transform>(transform.parent);
                skeleton.world[node.index] =
                    math::matrix_plain::mul(skeleton.local[node.index], parent.world_matrix);
            }

            for (auto& c : transform.children)
            {
                if (world.has_component<mmd_node>(c))
                    bfs.push(c);
            }
        }
    }
}

void mmd_animation::update_inherit(
    mmd_node& node,
    mmd_node_animation& animation,
    scene::transform& transform)
{
    auto& world = system<ecs::world>();

    auto& inherit = world.component<mmd_node>(node.inherit_node);
    auto& inherit_animation = world.component<mmd_node_animation>(node.inherit_node);
    auto& inherit_transform = world.component<scene::transform>(node.inherit_node);
    if (node.is_inherit_rotation)
    {
        math::float4 rotate;
        if (node.inherit_local_flag)
        {
            rotate = math::quaternion_plain::mul(
                inherit_animation.animation_rotate,
                inherit_transform.rotation);
        }
        else
        {
            if (inherit.inherit_node != ecs::INVALID_ENTITY)
            {
                rotate = inherit.inherit_rotate;
            }
            else
            {
                rotate = math::quaternion_plain::mul(
                    inherit_animation.animation_rotate,
                    inherit_transform.rotation);
            }
        }

        // IK
        if (world.has_component<mmd_ik_link>(node.inherit_node))
        {
            auto& link = world.component<mmd_ik_link>(node.inherit_node);
            rotate = math::quaternion_plain::mul(link.ik_rotate, rotate);
        }

        node.inherit_rotate = math::quaternion_plain::slerp(
            math::float4{0.0f, 0.0f, 0.0f, 1.0f},
            rotate,
            node.inherit_weight);
    }

    if (node.is_inherit_translation)
    {
        math::float3 translate;
        if (node.inherit_local_flag)
        {
            translate =
                math::vector_plain::sub(inherit_transform.position, inherit.initial_position);
        }
        else
        {
            if (inherit.inherit_node != ecs::INVALID_ENTITY)
            {
                translate = inherit.inherit_translate;
            }
            else
            {
                translate =
                    math::vector_plain::sub(inherit_transform.position, inherit.initial_position);
            }
        }
        node.inherit_translate = math::vector_plain::scale(translate, node.inherit_weight);
    }

    // update_local(node, transform);
}

void mmd_animation::update_ik(mmd_skeleton& skeleton, mmd_node& node, mmd_ik_solver& ik)
{
    auto& world = system<ecs::world>();

    for (auto& link_entity : ik.links)
    {
        auto& link = world.component<mmd_ik_link>(link_entity);
        link.prev_angle = {0.0f, 0.0f, 0.0f};
        link.ik_rotate = {0.0f, 0.0f, 0.0f, 1.0f};
        link.plane_mode_angle = 0.0f;

        update_transform(skeleton, world.component<mmd_node>(link_entity).index);
    }

    float max_dist = std::numeric_limits<float>::max();
    for (uint32_t i = 0; i < ik.loop_count; i++)
    {
        ik_solve_core(skeleton, node, ik, i);

        auto& target_node = world.component<mmd_node>(ik.ik_target);

        auto target_position = skeleton.world[target_node.index].row[3];
        auto ik_position = skeleton.world[node.index].row[3];

        float dist =
            math::vector_plain::length(math::vector_plain::sub(target_position, ik_position));
        if (dist < max_dist)
        {
            max_dist = dist;
            for (auto& link_entity : ik.links)
            {
                auto& link = world.component<mmd_ik_link>(link_entity);
                link.save_ik_rotate = link.ik_rotate;
            }
        }
        else
        {
            for (auto& link_entity : ik.links)
            {
                auto& link = world.component<mmd_ik_link>(link_entity);
                link.ik_rotate = link.save_ik_rotate;
                update_transform(skeleton, world.component<mmd_node>(link_entity).index);
            }
            break;
        }
    }
}

void mmd_animation::ik_solve_core(
    mmd_skeleton& skeleton,
    mmd_node& node,
    mmd_ik_solver& ik,
    std::size_t iteration)
{
    auto& world = system<ecs::world>();

    math::float4_simd ik_position = math::simd::load(skeleton.world[node.index].row[3]);
    for (std::size_t i = 0; i < ik.links.size(); ++i)
    {
        auto& link_entity = ik.links[i];
        if (link_entity == ik.ik_target)
            continue;

        auto& link_node = world.component<mmd_node>(link_entity);
        auto& link = world.component<mmd_ik_link>(link_entity);
        if (link.enable_limit)
        {
            if ((link.limit_min[0] != 0.0f || link.limit_max[0] != 0) &&
                (link.limit_min[1] == 0.0f || link.limit_max[1] == 0) &&
                (link.limit_min[2] == 0.0f || link.limit_max[2] == 0))
            {
                ik_solve_plane(skeleton, node, ik, iteration, i, 0);
                continue;
            }
            else if (
                (link.limit_min[0] == 0.0f || link.limit_max[0] == 0) &&
                (link.limit_min[1] != 0.0f || link.limit_max[1] != 0) &&
                (link.limit_min[2] == 0.0f || link.limit_max[2] == 0))
            {
                ik_solve_plane(skeleton, node, ik, iteration, i, 1);
                continue;
            }
            else if (
                (link.limit_min[0] == 0.0f || link.limit_max[0] == 0) &&
                (link.limit_min[1] == 0.0f || link.limit_max[1] == 0) &&
                (link.limit_min[2] != 0.0f || link.limit_max[2] != 0))
            {
                ik_solve_plane(skeleton, node, ik, iteration, i, 2);
                continue;
            }
        }

        auto target_node = world.component<mmd_node>(ik.ik_target);
        math::float4_simd target_position =
            math::simd::load(skeleton.world[target_node.index].row[3]);
        math::float4x4_simd link_inverse =
            math::matrix_simd::inverse(math::simd::load(skeleton.world[link_node.index]));

        auto link_ik_position = math::matrix_simd::mul(ik_position, link_inverse);
        auto link_target_position = math::matrix_simd::mul(target_position, link_inverse);

        auto link_ik_vec = math::vector_simd::normalize_vec3(link_ik_position);
        auto link_target_vec = math::vector_simd::normalize_vec3(link_target_position);

        auto dot = math::vector_simd::dot(link_ik_vec, link_target_vec);
        dot = math::clamp(dot, -1.0f, 1.0f);

        float angle = std::acos(dot);
        float angle_deg = math::to_degrees(angle);
        if (angle_deg < 1.0e-3f)
            continue;

        angle = math::clamp(angle, -ik.limit_angle, ik.limit_angle);
        auto cross =
            math::vector_simd::normalize(math::vector_simd::cross(link_target_vec, link_ik_vec));
        math::float4 rotate;
        math::simd::store(math::quaternion_simd::rotation_axis(cross, angle), rotate);

        auto& animation = world.component<mmd_node_animation>(link_entity);
        auto& transform = world.component<scene::transform>(link_entity);

        auto animation_rotate =
            math::quaternion_plain::mul(animation.animation_rotate, transform.rotation);
        auto link_rotate = math::quaternion_plain::mul(link.ik_rotate, animation_rotate);
        link_rotate = math::quaternion_plain::mul(link_rotate, rotate);

        link.ik_rotate = math::quaternion_plain::mul(
            link_rotate,
            math::quaternion_plain::inverse(animation_rotate));
        static std::size_t counter = 0;
        /*log::debug(
            "ik rotate: {} {} {},{},{},{}",
            counter++,
            link_node.name,
            link.ik_rotate[0],
            link.ik_rotate[1],
            link.ik_rotate[2],
            link.ik_rotate[3]);*/

        update_transform(skeleton, link_node.index);
    }
}

void mmd_animation::ik_solve_plane(
    mmd_skeleton& skeleton,
    mmd_node& node,
    mmd_ik_solver& ik,
    std::size_t iteration,
    std::size_t link_index,
    uint8_t axis)
{
    auto& world = system<ecs::world>();

    math::float3 rotate_axis;
    math::float3 plane;

    switch (axis)
    {
    case 0: // x axis
        rotate_axis = {1.0f, 0.0f, 0.0f};
        plane = {0.0f, 1.0f, 1.0f};
        break;
    case 1: // y axis
        rotate_axis = {0.0f, 1.0f, 0.0f};
        plane = {1.0f, 0.0f, 1.0f};
        break;
    case 2: // z axis
        rotate_axis = {0.0f, 0.0f, 1.0f};
        plane = {1.0f, 1.0f, 0.0f};
        break;
    default:
        return;
    }

    auto& link_entity = ik.links[link_index];
    auto& link_node = world.component<mmd_node>(link_entity);

    math::float4_simd ik_position = math::simd::load(skeleton.world[node.index].row[3]);

    auto target_node = world.component<mmd_node>(ik.ik_target);
    math::float4_simd target_position = math::simd::load(skeleton.world[target_node.index].row[3]);
    math::float4x4_simd link_inverse =
        math::matrix_simd::inverse(math::simd::load(skeleton.world[link_node.index]));

    auto link_ik_position = math::matrix_simd::mul(ik_position, link_inverse);
    auto link_target_position = math::matrix_simd::mul(target_position, link_inverse);

    math::float4 link_ik_vec;
    math::simd::store(math::vector_simd::normalize_vec3(link_ik_position), link_ik_vec);
    math::float4 link_target_vec;
    math::simd::store(math::vector_simd::normalize_vec3(link_target_position), link_target_vec);

    auto dot = math::vector_plain::dot(link_ik_vec, link_target_vec);
    dot = math::clamp(dot, -1.0f, 1.0f);

    float angle = std::acos(dot);
    angle = math::clamp(angle, -ik.limit_angle, ik.limit_angle);

    auto rotate1 = math::quaternion_plain::rotation_axis(rotate_axis, angle);
    auto target_vec1 = math::quaternion_plain::mul_vec(rotate1, link_target_vec);
    auto dot1 = math::vector_plain::dot(target_vec1, link_ik_vec);

    auto rotate2 = math::quaternion_plain::rotation_axis(rotate_axis, -angle);
    auto target_vec2 = math::quaternion_plain::mul_vec(rotate2, link_target_vec);
    auto dot2 = math::vector_plain::dot(target_vec2, link_ik_vec);

    auto& link = world.component<mmd_ik_link>(link_entity);
    auto new_angle = link.plane_mode_angle;
    if (dot1 > dot2)
        new_angle += angle;
    else
        new_angle -= angle;

    if (iteration == 0)
    {
        if (new_angle < link.limit_min[axis] || new_angle > link.limit_max[axis])
        {
            if (-new_angle > link.limit_min[axis] && -new_angle < link.limit_max[axis])
            {
                new_angle *= -1.0f;
            }
            else
            {
                auto halfRad = (link.limit_min[axis] + link.limit_max[axis]) * 0.5f;
                if (std::abs(halfRad - new_angle) > std::abs(halfRad + new_angle))
                {
                    new_angle *= -1.0f;
                }
            }
        }
    }

    new_angle = math::clamp(new_angle, link.limit_min[axis], link.limit_max[axis]);
    link.plane_mode_angle = new_angle;

    auto& animation = world.component<mmd_node_animation>(link_entity);
    auto& transform = world.component<scene::transform>(link_entity);
    auto animation_rotate =
        math::quaternion_plain::mul(animation.animation_rotate, transform.rotation);

    link.ik_rotate = math::quaternion_plain::rotation_axis(rotate_axis, new_angle);
    link.ik_rotate = math::quaternion_plain::mul(
        link.ik_rotate,
        math::quaternion_plain::inverse(animation_rotate));

    update_transform(skeleton, link_node.index);
}
} // namespace ash::sample::mmd