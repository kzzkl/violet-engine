#include "mmd_animation.hpp"
#include "scene/scene.hpp"

namespace ash::sample::mmd
{
bool mmd_animation::initialize(const dictionary& config)
{
    auto& world = system<ash::ecs::world>();
    world.register_component<mmd_node_animation>();
    world.register_component<mmd_ik_solver>();
    world.register_component<mmd_ik_link>();

    return true;
}

void mmd_animation::evaluate(float t, float weight)
{
    auto& world = system<ecs::world>();

    std::map<std::string, std::pair<mmd_node*, mmd_node_animation*>> m;
    world.view<mmd_node, mmd_node_animation>().each(
        [=, &m](mmd_node& node, mmd_node_animation& node_animation) {
            evaluate_node(node, node_animation, t, weight);
        });

    world.view<mmd_node, mmd_ik_solver>().each(
        [=](mmd_node& node, mmd_ik_solver& ik) { evaluate_ik(node, ik, t, weight); });

    world.view<mmd_morph_controler>().each(
        [=](ecs::entity entity, mmd_morph_controler& morph_controler) {
            evaluate_morph(entity, morph_controler, t);
        });
}

void mmd_animation::update(bool after_physics)
{
    auto& world = system<ecs::world>();
    world.view<mmd_skeleton>().each([&](ecs::entity entity, mmd_skeleton& skeleton) {
        update_local(skeleton, after_physics);
        update_world(skeleton, after_physics);
    });

    world.view<mmd_skeleton>().each([&](ecs::entity entity, mmd_skeleton& skeleton) {
        for (auto& node_entity : skeleton.sorted_nodes)
        {
            auto& node = world.component<mmd_node>(node_entity);
            if (after_physics == node.deform_after_physics &&
                node.inherit_node != ecs::INVALID_ENTITY)
            {
                update_inherit(node);
                update_world(skeleton, node_entity);
            }

            if (world.has_component<mmd_ik_solver>(node_entity))
            {
                auto& solver = world.component<mmd_ik_solver>(node_entity);
                if (solver.enable)
                {
                    update_ik(skeleton, node, solver);
                    update_world(skeleton, node_entity);
                }
            }
        }
    });
    world.view<mmd_skeleton>().each([&](ecs::entity entity, mmd_skeleton& skeleton) {
        update_local(skeleton, after_physics);
        update_world(skeleton, after_physics);
    });
}

void mmd_animation::evaluate_node(
    mmd_node& node,
    mmd_node_animation& node_animation,
    float t,
    float weight)
{
    if (node_animation.keys.empty())
    {
        node_animation.translation = {0.0f, 0.0f, 0.0f};
        node_animation.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        return;
    }

    auto bound =
        bound_key(node_animation.keys, static_cast<std::int32_t>(t), node_animation.offset);

    math::float4_simd translate;
    math::float4_simd rotate;

    if (bound == node_animation.keys.end())
    {
        translate = math::simd::load(node_animation.keys.back().translate);
        rotate = math::simd::load(node_animation.keys.back().rotate);
    }
    else if (bound == node_animation.keys.begin())
    {
        translate = math::simd::load(bound->translate);
        rotate = math::simd::load(bound->rotate);
    }
    else
    {
        const auto& key0 = *(bound - 1);
        const auto& key1 = *bound;

        float time = (t - key0.frame) / static_cast<float>(key1.frame - key0.frame);

        translate = math::vector_simd::lerp(
            math::simd::load(key0.translate),
            math::simd::load(key1.translate),
            math::simd::set(
                key1.tx_bezier.evaluate(time),
                key1.ty_bezier.evaluate(time),
                key1.tz_bezier.evaluate(time),
                0.0f));

        rotate = math::quaternion_simd::slerp(
            math::simd::load(key0.rotate),
            math::simd::load(key1.rotate),
            key1.r_bezier.evaluate(time));

        node_animation.offset = std::distance(node_animation.keys.cbegin(), bound);
    }

    if (weight != 1.0f)
    {
        rotate = math::quaternion_simd::slerp(
            math::simd::load(node_animation.base_rotation),
            rotate,
            weight);
        translate = math::vector_simd::lerp(
            math::simd::load(node_animation.base_translation),
            translate,
            weight);
    }

    math::simd::store(translate, node_animation.translation);
    math::simd::store(rotate, node_animation.rotation);
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
            ik.enable = ik.base_animation;
        else
            ik.enable = enable;
    }
}

void mmd_animation::evaluate_morph(
    ecs::entity entity,
    mmd_morph_controler& morph_controler,
    float t)
{
    std::memset(
        morph_controler.vertex_morph_result->pointer(),
        0,
        morph_controler.vertex_morph_result->size());

    for (auto& morph : morph_controler.morphs)
    {
        if (morph->keys.empty())
            continue;

        auto bound = bound_key(morph->keys, static_cast<std::int32_t>(t), morph->offset);

        float weight = 0.0f;
        if (bound == morph->keys.end())
        {
            weight = morph->keys.back().weight;
        }
        else if (bound == morph->keys.begin())
        {
            weight = bound->weight;
        }
        else
        {
            const auto& key0 = *(bound - 1);
            const auto& key1 = *bound;

            float offset = (t - key0.frame) / (key1.frame - key0.frame);
            weight = key0.weight + (key1.weight - key0.weight) * offset;

            morph->offset = std::distance(morph->keys.cbegin(), bound);
        }

        if (weight != 0.0f)
            morph->evaluate(weight, entity);
    }
}

void mmd_animation::update_local(mmd_skeleton& skeleton, bool after_physics)
{
    auto& world = system<ecs::world>();

    for (auto& entity : skeleton.sorted_nodes)
    {
        auto& node = world.component<mmd_node>(entity);
        if (after_physics == node.deform_after_physics)
            update_local(skeleton, entity);
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

        auto& link = world.component<core::link>(node_entity);

        math::float4x4_simd parent_to_world;
        if (world.has_component<mmd_node>(link.parent))
        {
            auto& parent = world.component<mmd_node>(link.parent);
            parent_to_world = math::simd::load(skeleton.world[parent.index]);
        }
        else
        {
            auto& parent = world.component<scene::transform>(link.parent);
            parent_to_world = math::simd::load(parent.to_world());
        }

        math::float4x4_simd to_parent = math::simd::load(skeleton.local[node.index]);
        math::simd::store(
            math::matrix_simd::mul(to_parent, parent_to_world),
            skeleton.world[node.index]);
    }
}

void mmd_animation::update_local(mmd_skeleton& skeleton, ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& transform = world.component<scene::transform>(entity);
    auto& animation = world.component<mmd_node_animation>(entity);
    auto& node = world.component<mmd_node>(entity);

    math::float4_simd translate = math::vector_simd::add(
        math::simd::load(animation.translation),
        math::simd::load(transform.position()));
    if (node.is_inherit_translation)
        translate = math::vector_simd::add(translate, math::simd::load(node.inherit_translation));

    math::float4_simd rotate = math::quaternion_simd::mul(
        math::simd::load(animation.rotation),
        math::simd::load(transform.rotation()));
    if (world.has_component<mmd_ik_link>(entity))
    {
        auto& ik_link = world.component<mmd_ik_link>(entity);
        rotate = math::quaternion_simd::mul(math::simd::load(ik_link.ik_rotate), rotate);
    }
    if (node.is_inherit_rotation)
        rotate = math::quaternion_simd::mul(rotate, math::simd::load(node.inherit_rotation));

    math::simd::store(
        math::matrix_simd::affine_transform(math::simd::load(transform.scale()), rotate, translate),
        skeleton.local[node.index]);
}

void mmd_animation::update_world(mmd_skeleton& skeleton, ecs::entity entity)
{
    auto& world = system<ecs::world>();

    std::queue<ecs::entity> bfs;
    bfs.push(entity);

    while (!bfs.empty())
    {
        ecs::entity node_entity = bfs.front();
        bfs.pop();

        auto& node = world.component<mmd_node>(node_entity);
        auto& link = world.component<core::link>(node_entity);
        auto& transform = world.component<scene::transform>(node_entity);

        math::float4x4_simd parent_to_world;
        if (world.has_component<mmd_node>(link.parent))
        {
            auto& parent = world.component<mmd_node>(link.parent);
            parent_to_world = math::simd::load(skeleton.world[parent.index]);
        }
        else
        {
            auto& parent = world.component<scene::transform>(link.parent);
            parent_to_world = math::simd::load(parent.to_world());
        }

        math::float4x4_simd to_parent = math::simd::load(skeleton.local[node.index]);
        math::simd::store(
            math::matrix_simd::mul(to_parent, parent_to_world),
            skeleton.world[node.index]);

        for (auto& c : link.children)
        {
            if (world.has_component<mmd_node>(c))
                bfs.push(c);
        }
    }
}

void mmd_animation::update_inherit(mmd_node& node)
{
    auto& world = system<ecs::world>();

    auto& inherit = world.component<mmd_node>(node.inherit_node);
    auto& inherit_animation = world.component<mmd_node_animation>(node.inherit_node);
    auto& inherit_transform = world.component<scene::transform>(node.inherit_node);
    if (node.is_inherit_rotation)
    {
        math::float4_simd rotate;
        if (!node.inherit_local_flag && inherit.inherit_node != ecs::INVALID_ENTITY)
        {
            rotate = math::simd::load(inherit.inherit_rotation);
        }
        else
        {
            rotate = math::quaternion_simd::mul(
                math::simd::load(inherit_animation.rotation),
                math::simd::load(inherit_transform.rotation()));
        }

        // IK
        if (world.has_component<mmd_ik_link>(node.inherit_node))
        {
            auto& link = world.component<mmd_ik_link>(node.inherit_node);
            rotate = math::quaternion_simd::mul(math::simd::load(link.ik_rotate), rotate);
        }

        rotate = math::quaternion_simd::slerp(
            math::quaternion_simd::identity(),
            rotate,
            node.inherit_weight);
        math::simd::store(rotate, node.inherit_rotation);
    }

    if (node.is_inherit_translation)
    {
        math::float4_simd translate;
        if (!node.inherit_local_flag && inherit.inherit_node != ecs::INVALID_ENTITY)
        {
            translate = math::simd::load(inherit.inherit_translation);
        }
        else
        {
            translate = math::vector_simd::sub(
                math::simd::load(inherit_transform.position()),
                math::simd::load(inherit.initial_position));
        }
        translate = math::vector_simd::mul(translate, node.inherit_weight);
        math::simd::store(translate, node.inherit_translation);
    }
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
        update_local(skeleton, link_entity);
        update_world(skeleton, link_entity);
    }

    float max_dist = std::numeric_limits<float>::max();
    for (uint32_t i = 0; i < ik.loop_count; i++)
    {
        ik_solve_core(skeleton, node, ik, i);

        auto& target_node = world.component<mmd_node>(ik.ik_target);

        math::float4_simd target_position =
            math::simd::load(skeleton.world[target_node.index].row[3]);
        math::float4_simd ik_position = math::simd::load(skeleton.world[node.index].row[3]);

        float dist =
            math::vector_simd::length(math::vector_simd::sub(target_position, ik_position));
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
                update_local(skeleton, link_entity);
                update_world(skeleton, link_entity);
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
    auto& target_node = world.component<mmd_node>(ik.ik_target);
    math::float4_simd target_position = math::simd::load(skeleton.world[target_node.index].row[3]);
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

        math::float4x4_simd link_inverse =
            math::matrix_simd::inverse(math::simd::load(skeleton.world[link_node.index]));

        math::float4_simd link_ik_position = math::matrix_simd::mul(ik_position, link_inverse);
        math::float4_simd link_target_position =
            math::matrix_simd::mul(target_position, link_inverse);

        math::float4_simd link_ik_vec = math::vector_simd::normalize(link_ik_position);
        math::float4_simd link_target_vec = math::vector_simd::normalize(link_target_position);

        float dot = math::vector_simd::dot(link_ik_vec, link_target_vec);
        dot = math::clamp(dot, -1.0f, 1.0f);

        float angle = std::acos(dot);
        float angle_deg = math::to_degrees(angle);
        if (angle_deg < 1.0e-3f)
            continue;

        angle = math::clamp(angle, -ik.limit_angle, ik.limit_angle);
        math::float4_simd cross =
            math::vector_simd::normalize(math::vector_simd::cross(link_target_vec, link_ik_vec));
        math::float4_simd rotate = math::quaternion_simd::rotation_axis(cross, angle);

        auto& animation = world.component<mmd_node_animation>(link_entity);
        auto& transform = world.component<scene::transform>(link_entity);

        math::float4_simd rotation = math::quaternion_simd::mul(
            math::simd::load(animation.rotation),
            math::simd::load(transform.rotation()));

        math::float4_simd link_rotate = math::simd::load(link.ik_rotate);
        link_rotate = math::quaternion_simd::mul(link_rotate, rotation);
        link_rotate = math::quaternion_simd::mul(link_rotate, rotate);
        link_rotate =
            math::quaternion_simd::mul(link_rotate, math::quaternion_simd::inverse(rotation));
        math::simd::store(link_rotate, link.ik_rotate);

        update_local(skeleton, link_entity);
        update_world(skeleton, link_entity);
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

    math::float4_simd rotate_axis;
    switch (axis)
    {
    case 0: // x axis
        rotate_axis = math::simd::identity_row<1>();
        break;
    case 1: // y axis
        rotate_axis = math::simd::identity_row<2>();
        break;
    case 2: // z axis
        rotate_axis = math::simd::identity_row<3>();
        break;
    default:
        return;
    }

    auto& link_entity = ik.links[link_index];
    auto& link_node = world.component<mmd_node>(link_entity);

    math::float4_simd ik_position = math::simd::load(skeleton.world[node.index].row[3]);

    auto& target_node = world.component<mmd_node>(ik.ik_target);
    math::float4_simd target_position = math::simd::load(skeleton.world[target_node.index].row[3]);
    math::float4x4_simd link_inverse =
        math::matrix_simd::inverse(math::simd::load(skeleton.world[link_node.index]));

    math::float4_simd link_ik_vec = math::matrix_simd::mul(ik_position, link_inverse);
    link_ik_vec = math::vector_simd::normalize_vec3(link_ik_vec);

    math::float4_simd link_target_vec = math::matrix_simd::mul(target_position, link_inverse);
    link_target_vec = math::vector_simd::normalize_vec3(link_target_vec);

    float dot = math::vector_simd::dot(link_ik_vec, link_target_vec);
    dot = math::clamp(dot, -1.0f, 1.0f);

    float angle = std::acos(dot);
    angle = math::clamp(angle, -ik.limit_angle, ik.limit_angle);

    math::float4_simd rotate1 = math::quaternion_simd::rotation_axis(rotate_axis, angle);
    math::float4_simd target_vec1 = math::quaternion_simd::mul_vec(rotate1, link_target_vec);
    float dot1 = math::vector_simd::dot(target_vec1, link_ik_vec);

    math::float4_simd rotate2 = math::quaternion_simd::rotation_axis(rotate_axis, -angle);
    math::float4_simd target_vec2 = math::quaternion_simd::mul_vec(rotate2, link_target_vec);
    float dot2 = math::vector_simd::dot(target_vec2, link_ik_vec);

    auto& link = world.component<mmd_ik_link>(link_entity);
    float new_angle = link.plane_mode_angle;
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
                    new_angle *= -1.0f;
            }
        }
    }

    new_angle = math::clamp(new_angle, link.limit_min[axis], link.limit_max[axis]);
    link.plane_mode_angle = new_angle;

    auto& animation = world.component<mmd_node_animation>(link_entity);
    auto& transform = world.component<scene::transform>(link_entity);
    math::float4_simd rotation = math::quaternion_simd::mul(
        math::simd::load(animation.rotation),
        math::simd::load(transform.rotation()));

    math::float4_simd link_rotate = math::quaternion_simd::rotation_axis(rotate_axis, new_angle);
    link_rotate = math::quaternion_simd::mul(link_rotate, math::quaternion_simd::inverse(rotation));

    math::simd::store(link_rotate, link.ik_rotate);

    update_local(skeleton, link_entity);
    update_world(skeleton, link_entity);
}
} // namespace ash::sample::mmd