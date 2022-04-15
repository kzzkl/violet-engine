#include "mmd_viewer.hpp"
#include "scene.hpp"
#include "transform.hpp"

namespace ash::sample::mmd
{
bool mmd_viewer::initialize(const dictionary& config)
{
    auto& world = system<ash::ecs::world>();
    world.register_component<mmd_bone>();
    world.register_component<mmd_skeleton>();
    world.register_component<mmd_animation>();

    m_skeleton_view = world.make_view<mmd_skeleton>();
    m_bone_view = world.make_view<mmd_bone, scene::transform>();

    m_loader = std::make_unique<mmd_loader>(
        world,
        system<graphics::graphics>(),
        system<scene::scene>(),
        system<physics::physics>());
    m_loader->initialize();

    m_ik_solver = std::make_unique<mmd_ik_solver>(world);

    return true;
}

ash::ecs::entity mmd_viewer::load_mmd(
    std::string_view name,
    std::string_view pmx,
    std::string_view vmd)
{
    mmd_resource resource;
    if (m_loader->load(resource, pmx, vmd))
    {
        ecs::entity result = resource.root;
        m_resources[name.data()] = std::move(resource);
        return result;
    }
    else
    {
        return ecs::INVALID_ENTITY;
    }
}

void mmd_viewer::update()
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();

    m_skeleton_view->each([&](ecs::entity entity, mmd_skeleton& skeleton) {
        reset(entity);
        update_animation(entity, 0.0f, 1.0f);
        update_node(entity, false);
    });

    //system<physics::physics>().simulation();

    m_skeleton_view->each(
        [&](ecs::entity entity, mmd_skeleton& skeleton) { update_node(entity, true); });

    m_skeleton_view->each([&](mmd_skeleton& skeleton) {
        for (std::size_t i = 0; i < skeleton.nodes.size(); ++i)
        {
            math::float4x4_simd to_world =
                math::simd::load(world.component<scene::transform>(skeleton.nodes[i]).world_matrix);
            math::float4x4_simd initial =
                math::simd::load(world.component<mmd_bone>(skeleton.nodes[i]).initial_inverse);

            math::float4x4_simd final_transform = math::matrix_simd::mul(initial, to_world);
            math::simd::store(final_transform, skeleton.transform[i]);
        }

        skeleton.parameter->set(0, skeleton.transform.data(), skeleton.transform.size());
    });
}

void mmd_viewer::update_animation(ecs::entity entity, float t, float weight)
{
    auto& world = system<ecs::world>();
    auto& skeleton = world.component<mmd_skeleton>(entity);

    for (auto& node_entity : skeleton.nodes)
    {
        auto& node = world.component<mmd_bone>(node_entity);
        auto& animation = world.component<mmd_animation>(node_entity);

        if (animation.keys.empty())
        {
            node.enable_ik_solver = true;
            node.animation_translate = {0.0f, 0.0f, 0.0f};
            node.animation_rotate = {0.0f, 0.0f, 0.0f, 1.0f};
        }
        else
        {
            node.enable_ik_solver = false;
            node.animation_translate = animation.keys[0].translate;
            node.animation_rotate = animation.keys[0].rotate;
        }
    }
}

void mmd_viewer::update_node(ecs::entity entity, bool after_physics)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& skeleton = world.component<mmd_skeleton>(entity);

    sync_node(after_physics);
    scene.sync_local(entity);

    for (auto& node_entity : skeleton.sorted_nodes)
    {
        auto& node = world.component<mmd_bone>(node_entity);

        if (node.deform_after_physics != after_physics)
            continue;

        if (node.inherit_node != ecs::INVALID_ENTITY)
            update_inherit(node_entity);

        if (node.enable_ik)
            update_ik(node_entity);
    }

    scene.sync_local(entity);
}

void mmd_viewer::reset(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& skeleton = world.component<mmd_skeleton>(entity);

    for (auto& node_entity : skeleton.nodes)
    {
        auto& node = world.component<mmd_bone>(node_entity);
        auto& node_transform = world.component<scene::transform>(node_entity);
        node_transform.dirty = true;

        node_transform.position = node.initial_position;
        node_transform.rotation = node.initial_rotation;

        node.inherit_translate = {0.0f, 0.0f, 0.0f};
        node.inherit_rotate = {0.0f, 0.0f, 0.0f, 1.0f};
    }
}

void mmd_viewer::update_inherit(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& node = world.component<mmd_bone>(entity);

    auto& inherit_node = world.component<mmd_bone>(node.inherit_node);
    auto& inherit_node_transform = world.component<scene::transform>(node.inherit_node);
    if (node.inherit_rotation_flag)
    {
        math::float4 rotate;
        if (node.inherit_local_flag)
        {
            rotate = math::quaternion_plain::mul(
                inherit_node.animation_rotate,
                inherit_node_transform.rotation);
        }
        else
        {
            if (inherit_node.inherit_node != ecs::INVALID_ENTITY)
            {
                rotate = inherit_node.inherit_rotate;
            }
            else
            {
                rotate = math::quaternion_plain::mul(
                    inherit_node.animation_rotate,
                    inherit_node_transform.rotation);
            }
        }

        node.inherit_rotate = math::quaternion_plain::slerp(
            math::float4{0.0f, 0.0f, 0.0f, 1.0f},
            rotate,
            node.inherit_weight);
    }

    if (node.inherit_translation_flag)
    {
        math::float3 translate;
        if (node.inherit_local_flag)
        {
            translate = math::vector_plain::sub(
                inherit_node_transform.position,
                inherit_node.initial_position);
        }
        else
        {
            if (inherit_node.inherit_node != ecs::INVALID_ENTITY)
            {
                translate = inherit_node.inherit_translate;
            }
            else
            {
                translate = math::vector_plain::sub(
                    inherit_node_transform.position,
                    inherit_node.initial_position);
            }
        }
        node.inherit_translate = math::vector_plain::scale(translate, node.inherit_weight);
    }
}

void mmd_viewer::update_ik(ecs::entity entity)
{
    m_ik_solver->solve(entity);
}

void mmd_viewer::sync_node(bool after_physics)
{
    m_bone_view->each(
        [after_physics](ecs::entity entity, mmd_bone& node, scene::transform& transform) {
            if (node.deform_after_physics != after_physics)
                return;

            transform.dirty = true;

            math::float3 translate =
                math::vector_plain::add(node.animation_translate, transform.position);
            if (node.inherit_translation_flag)
                math::vector_plain::add(node.inherit_translate, translate);
            transform.position = translate;

            math::float4 rotate =
                math::quaternion_plain::mul(node.animation_rotate, transform.rotation);
            if (node.enable_ik)
                rotate = math::quaternion_plain::mul(node.ik_rotate, rotate);
            if (node.inherit_rotation_flag)
                rotate = math::quaternion_plain::mul(rotate, node.inherit_rotate);
            transform.rotation = rotate;
        });
}
} // namespace ash::sample::mmd