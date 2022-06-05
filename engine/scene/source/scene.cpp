#include "scene/scene.hpp"
#include "core/relation.hpp"
#include "scene/scene_event.hpp"

namespace ash::scene
{
scene::scene() : system_base("scene"), m_view(nullptr)
{
}

bool scene::initialize(const dictionary& config)
{
    auto& world = system<ecs::world>();
    world.register_component<transform>();
    m_view = world.make_view<transform>();

    m_root = world.create("scene");
    world.add<core::link, transform>(m_root);

    auto& root_transform = world.component<transform>(m_root);
    root_transform.dirty = false;
    root_transform.in_scene = true;

    auto& event = system<core::event>();
    event.register_event<event_enter_scene>();
    event.register_event<event_exit_scene>();

    event.subscribe<core::event_link>("scene", [this](ecs::entity entity, core::link& link) {
        on_entity_link(entity, link);
    });

    event.subscribe<core::event_unlink>("scene", [this](ecs::entity entity, core::link& link) {
        on_entity_unlink(entity, link);
    });

    return true;
}

void scene::sync_local()
{
    sync_local(m_root);
}

void scene::sync_local(ecs::entity root)
{
    auto& world = system<ecs::world>();

    std::queue<ecs::entity> dirty_entities;
    auto find_dirty = [&](ecs::entity entity) {
        if (!world.has_component<transform>(entity))
            return false;

        auto& node_transform = world.component<transform>(entity);
        if (node_transform.dirty)
        {
            dirty_entities.push(entity);
            return false;
        }
        else
        {
            return true;
        }
    };
    system<core::relation>().each_bfs(root, find_dirty);

    auto update_local = [&](ecs::entity entity) {
        if (!world.has_component<transform>(entity))
            return false;

        auto& node_transform = world.component<transform>(entity);
        auto& node_link = world.component<core::link>(entity);

        // Update to parent matrix.
        math::float4_simd scale = math::simd::load(node_transform.scaling);
        math::float4_simd rotation = math::simd::load(node_transform.rotation);
        math::float4_simd translation = math::simd::load(node_transform.position);

        math::float4x4_simd to_parent =
            math::matrix_simd::affine_transform(scale, rotation, translation);
        math::simd::store(to_parent, node_transform.parent_matrix);

        // Update to world matrix.
        if (node_link.parent != ecs::INVALID_ENTITY)
        {
            auto& parent = world.component<transform>(node_link.parent);
            math::float4x4_simd parent_to_world = math::simd::load(parent.world_matrix);
            math::float4x4_simd to_world = math::matrix_simd::mul(to_parent, parent_to_world);
            math::simd::store(to_world, node_transform.world_matrix);
        }
        else
        {
            node_transform.world_matrix = node_transform.parent_matrix;
        }

        ++node_transform.sync_count;
        node_transform.dirty = false;

        return true;
    };

    while (!dirty_entities.empty())
    {
        system<core::relation>().each_bfs(dirty_entities.front(), update_local);
        dirty_entities.pop();
    }
}

void scene::sync_world()
{
    sync_world(m_root);
}

void scene::sync_world(ecs::entity root)
{
    auto& world = system<ecs::world>();

    std::queue<ecs::entity> dirty_entities;
    auto find_dirty = [&](ecs::entity entity) {
        if (!world.has_component<transform>(entity))
            return false;

        auto& node_transform = world.component<transform>(entity);
        if (node_transform.dirty)
        {
            dirty_entities.push(entity);
            return false;
        }
        else
        {
            return true;
        }
    };
    system<core::relation>().each_bfs(root, find_dirty);

    auto update_world = [&](ecs::entity entity) {
        if (!world.has_component<transform>(entity))
            return false;

        auto& node_transform = world.component<transform>(entity);
        auto& node_link = world.component<core::link>(entity);
        auto& parent = world.component<transform>(node_link.parent);

        math::float4x4_simd parent_to_world = math::simd::load(parent.world_matrix);
        math::float4x4_simd to_world = math::simd::load(node_transform.world_matrix);
        math::float4x4_simd to_parent =
            math::matrix_simd::mul(to_world, math::matrix_simd::inverse(parent_to_world));

        // Update transform data.
        math::float4_simd scaling, rotation, position;
        math::matrix_simd::decompose(to_parent, scaling, rotation, position);

        math::simd::store(to_parent, node_transform.parent_matrix);
        math::simd::store(scaling, node_transform.scaling);
        math::simd::store(rotation, node_transform.rotation);
        math::simd::store(position, node_transform.position);

        ++node_transform.sync_count;
        node_transform.dirty = false;

        return true;
    };

    while (!dirty_entities.empty())
    {
        system<core::relation>().each_bfs(dirty_entities.front(), update_world);
        dirty_entities.pop();
    }
}

void scene::reset_sync_counter()
{
    m_view->each([](transform& transform) { transform.sync_count = 0; });
}

void scene::on_entity_link(ecs::entity entity, core::link& link)
{
    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    if (world.has_component<transform>(entity) && world.has_component<transform>(link.parent))
    {
        auto& child = world.component<transform>(entity);
        auto& parent = world.component<transform>(link.parent);

        if (child.in_scene != parent.in_scene)
        {
            child.in_scene = parent.in_scene;

            if (child.in_scene)
                event.publish<event_enter_scene>(entity);
            else
                event.publish<event_exit_scene>(entity);
        }
    }
}

void scene::on_entity_unlink(ecs::entity entity, core::link& link)
{
    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    if (world.has_component<transform>(entity))
    {
        auto& node_transform = world.component<transform>(entity);
        if (node_transform.in_scene)
        {
            node_transform.in_scene = false;
            event.publish<event_exit_scene>(entity);
        }
    }
}
} // namespace ash::scene