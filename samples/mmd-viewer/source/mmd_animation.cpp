#include "mmd_animation.hpp"
#include "scene.hpp"

namespace ash::sample::mmd
{
bool mmd_animation::initialize(const dictionary& config)
{
    auto& world = system<ash::ecs::world>();
    world.register_component<mmd_node_animation>();
    world.register_component<mmd_ik_animation>();

    m_view = world.make_view<mmd_skeleton>();
    m_node_view = world.make_view<mmd_node, mmd_node_animation>();
    m_ik_view = world.make_view<mmd_node, mmd_ik_animation>();
    m_transform_view = world.make_view<mmd_node, scene::transform>();

    return true;
}

void mmd_animation::evaluate(float t, float weight)
{
    std::map<std::string, std::pair<mmd_node*, mmd_node_animation*>> m;
    m_node_view->each([=, &m](mmd_node& node, mmd_node_animation& node_animation) {
        //evaluate_node(node, node_animation, t, weight);
        m[node.name] = { &node, &node_animation };
    });

    for (auto [key, value] : m)
    {
        evaluate_node(*value.first, *value.second, t, weight);
        //log::debug("{} {},{},{},{}", key, value[0], value[1], value[2], value[3]);
    }

    //m_ik_view->each([=](mmd_node& node, mmd_ik_animation& ik_animation) {
    //    evaluate_ik(node, ik_animation, t, weight);
    //});
}

void mmd_animation::evaluate_node(
    mmd_node& node,
    mmd_node_animation& node_animation,
    float t,
    float weight)
{
    if (node_animation.keys.empty())
    {
        node.animation_translate = {0.0f, 0.0f, 0.0f};
        node.animation_rotate = {0.0f, 0.0f, 0.0f, 1.0f};
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
            // TODO
            /*const auto& key0 = *(boundIt - 1);
            const auto& key1 = *boundIt;

            float timeRange = float(key1.m_time - key0.m_time);
            float time = (t - float(key0.m_time)) / timeRange;
            float tx_x = key0.m_txBezier.FindBezierX(time);
            float ty_x = key0.m_tyBezier.FindBezierX(time);
            float tz_x = key0.m_tzBezier.FindBezierX(time);
            float rot_x = key0.m_rotBezier.FindBezierX(time);
            float tx_y = key0.m_txBezier.EvalY(tx_x);
            float ty_y = key0.m_tyBezier.EvalY(ty_x);
            float tz_y = key0.m_tzBezier.EvalY(tz_x);
            float rot_y = key0.m_rotBezier.EvalY(rot_x);

            vt = glm::mix(key0.m_translate, key1.m_translate, glm::vec3(tx_y, ty_y, tz_y));
            q = glm::slerp(key0.m_rotate, key1.m_rotate, rot_y);

            m_startKeyIndex = std::distance(m_keys.cbegin(), boundIt);*/
        }
    }

    if (weight == 1.0f)
    {
        node.animation_translate = translate;
        node.animation_rotate = rotate;
    }
    else
    {
        node.animation_rotate =
            math::quaternion_plain::slerp(node.base_animation_rotate, rotate, weight);
        node.animation_translate =
            math::vector_plain::mix(node.base_animation_translate, translate, weight);
    }

    //log::debug("{} {},{},{},{}", node.name, node.animation_rotate[0], node.animation_rotate[1], node.animation_rotate[2], node.animation_rotate[3]);
}

void mmd_animation::update(bool after_physics)
{
    auto& world = system<ecs::world>();
    m_view->each([&](ecs::entity entity, mmd_skeleton& skeleton) {
        for (auto& node_entity : skeleton.sorted_nodes)
        {
            auto& node = world.component<mmd_node>(node_entity);
            auto& transform = world.component<scene::transform>(node_entity);
            if (after_physics == node.deform_after_physics)
            {
                update_local(node, transform);
                update_world(node, transform);
            }
        }
    });
    //system<scene::scene>().sync_local();

    m_view->each([&](ecs::entity entity, mmd_skeleton& skeleton) {
        for (auto& node_entity : skeleton.sorted_nodes)
        {
            auto& node = world.component<mmd_node>(node_entity);
            auto& transform = world.component<scene::transform>(node_entity);
            if (after_physics == node.deform_after_physics &&
                node.inherit_node != ecs::INVALID_ENTITY)
            {
                update_inherit(node, transform);
                // log::debug("{} {},{},{},{}", node.name,
                // node.inherit_rotate[0],node.inherit_rotate[1],node.inherit_rotate[2],node.inherit_rotate[3]);
            }
        }
    });
    m_view->each([&](ecs::entity entity, mmd_skeleton& skeleton) {
        for (auto& node_entity : skeleton.sorted_nodes)
        {
            auto& node = world.component<mmd_node>(node_entity);
            auto& transform = world.component<scene::transform>(node_entity);
            if (after_physics == node.deform_after_physics)
                update_local(node, transform);
        }
    });
    system<scene::scene>().sync_local();

    m_view->each([&](ecs::entity entity, mmd_skeleton& skeleton) {
        for (auto& node_entity : skeleton.sorted_nodes)
        {
            auto& node = world.component<mmd_node>(node_entity);
            auto& transform = world.component<scene::transform>(node_entity);
            // if (after_physics == node.deform_after_physics && node.enable_ik)
            //     update_ik(node, transform);
        }
    });
    system<scene::scene>().sync_local();

    // sorted order
    /*m_transform_view->each([after_physics, this](mmd_node& node, scene::transform& transform) {
        if (after_physics == node.deform_after_physics)
            update_local(node, transform);
    });
    system<scene::scene>().sync_local();

    m_transform_view->each([after_physics, this](mmd_node& node, scene::transform& transform) {
        if (after_physics == node.deform_after_physics && node.inherit_node != ecs::INVALID_ENTITY)
            update_inherit(node, transform);
    });
    m_transform_view->each([after_physics, this](mmd_node& node, scene::transform& transform) {
        if (after_physics == node.deform_after_physics)
            update_local(node, transform);
    });
    system<scene::scene>().sync_local();

    m_transform_view->each([after_physics, this](mmd_node& node, scene::transform& transform) {
        if (after_physics == node.deform_after_physics && node.enable_ik)
            update_ik(node, transform);
    });
    system<scene::scene>().sync_local();*/
}

void mmd_animation::evaluate_ik(
    mmd_node& node,
    mmd_ik_animation& ik_animation,
    float t,
    float weight)
{
    if (ik_animation.keys.empty())
    {
        node.enable_ik_solver = true;
        return;
    }

    auto bound = bound_key(ik_animation.keys, static_cast<std::int32_t>(t), ik_animation.offset);
    bool enable = true;
    if (bound == ik_animation.keys.end())
    {
        enable = ik_animation.keys.back().enable;
    }
    else
    {
        // TODO
        /*enable = ik_animation.keys.front().enable;
        if (bound != ik_animation.keys.begin())
        {
            const auto& key = *(boundIt - 1);
            enable = key.m_enable;

            m_startKeyIndex = std::distance(m_keys.cbegin(), boundIt);
        }*/
    }

    if (weight == 1.0f)
    {
        node.enable_ik_solver = enable;
    }
    else
    {
        if (weight < 1.0f)
            node.enable_ik_solver = true;
        else
            node.enable_ik_solver = enable;
    }
}

void mmd_animation::update_local(mmd_node& node, scene::transform& transform)
{
    math::float3 translate = math::vector_plain::add(node.animation_translate, transform.position);
    if (node.is_inherit_translation)
        translate = math::vector_plain::add(translate, node.inherit_translate);

    math::float4 rotate = math::quaternion_plain::mul(node.animation_rotate, transform.rotation);
    if (node.enable_ik)
        rotate = math::quaternion_plain::mul(node.ik_rotate, rotate);
    if (node.is_inherit_rotation)
        rotate = math::quaternion_plain::mul(rotate, node.inherit_rotate);

    node.local = math::matrix_plain::affine_transform(transform.scaling, rotate, translate);

    transform.position = translate;
    transform.rotation = rotate;
    transform.dirty = true;
}

void mmd_animation::update_world(mmd_node& node, scene::transform& transform)
{
    auto& world = system<ecs::world>();
    if (world.has_component<mmd_node>(transform.parent))
    {
        auto& parent = world.component<mmd_node>(transform.parent);
        node.world = math::matrix_plain::mul(node.local, parent.world);
    }
    else
    {
        auto& parent = world.component<scene::transform>(transform.parent);
        node.world = math::matrix_plain::mul(node.local, parent.world_matrix);
    }
}

void mmd_animation::update_inherit(mmd_node& node, scene::transform& transform)
{
    auto& world = system<ecs::world>();

    auto& inherit = world.component<mmd_node>(node.inherit_node);
    auto& inherit_transform = world.component<scene::transform>(node.inherit_node);
    if (node.is_inherit_rotation)
    {
        math::float4 rotate;
        if (node.inherit_local_flag)
        {
            rotate =
                math::quaternion_plain::mul(inherit.animation_rotate, inherit_transform.rotation);
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
                    inherit.animation_rotate,
                    inherit_transform.rotation);
            }
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

    //update_local(node, transform);
}

void mmd_animation::update_ik(mmd_node& node, scene::transform& transform)
{
}
} // namespace ash::sample::mmd