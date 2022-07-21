#include "scene/scene.hpp"
#include "core/relation.hpp"
#include "core/relation_event.hpp"
#include "scene/bounding_box.hpp"
#include "scene/scene_event.hpp"

namespace ash::scene
{
scene::scene() : system_base("scene")
{
}

bool scene::initialize(const dictionary& config)
{
    auto& world = system<ecs::world>();
    world.register_component<transform>();
    world.register_component<bounding_box>();

    m_root = world.create("scene");
    world.add<core::link, transform>(m_root);

    auto& root_transform = world.component<transform>(m_root);
    root_transform.in_scene(true);

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

void scene::on_end_frame()
{
    auto& world = system<ecs::world>();
    world.view<transform>().each([](transform& transform) { transform.reset_sync_count(); });
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
        if (node_transform.dirty())
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

        ASH_ASSERT(node_link.parent != ecs::INVALID_ENTITY);

        // Update to parent matrix.
        math::float4_simd scale = math::simd::load(node_transform.scale());
        math::float4_simd rotation = math::simd::load(node_transform.rotation());
        math::float4_simd translation = math::simd::load(node_transform.position());
        math::float4x4_simd to_parent =
            math::matrix_simd::affine_transform(scale, rotation, translation);

        // Update to world matrix.
        auto& parent = world.component<transform>(node_link.parent);
        math::float4x4_simd parent_to_world = math::simd::load(parent.to_world());
        math::float4x4_simd to_world = math::matrix_simd::mul(to_parent, parent_to_world);

        node_transform.sync(to_parent, to_world);

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
        if (node_transform.dirty())
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

        math::float4x4_simd parent_to_world = math::simd::load(parent.to_world());
        math::float4x4_simd to_world = math::simd::load(node_transform.to_world());
        math::float4x4_simd to_parent =
            math::matrix_simd::mul(to_world, math::matrix_simd::inverse(parent_to_world));
        node_transform.sync(to_parent);

        return true;
    };

    while (!dirty_entities.empty())
    {
        system<core::relation>().each_bfs(dirty_entities.front(), update_world);
        dirty_entities.pop();
    }
}

void scene::frustum_culling(const std::vector<math::float4>& frustum)
{
    auto& world = system<ecs::world>();

    world.view<transform, bounding_box>().each(
        [this](transform& transform, bounding_box& bounding_box) {
            if (bounding_box.dynamic() && transform.sync_count() != 0)
            {
                if (bounding_box.transform(transform.to_world()))
                {
                    std::size_t new_proxy_id =
                        m_dynamic_bvh.update(bounding_box.proxy_id(), bounding_box.aabb());
                    bounding_box.proxy_id(new_proxy_id);
                }
            }
        });

    //m_static_bvh.frustum_culling(frustum);
    m_dynamic_bvh.frustum_culling(frustum);

    world.view<transform, bounding_box>().each(
        [this](transform& transform, bounding_box& bounding_box) {
            if (bounding_box.dynamic())
                bounding_box.visible(m_dynamic_bvh.visible(bounding_box.proxy_id()));
            else
                bounding_box.visible(m_static_bvh.visible(bounding_box.proxy_id()));
        });
}

void scene::on_entity_link(ecs::entity entity, core::link& link)
{
    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    if (world.has_component<transform>(entity) && world.has_component<transform>(link.parent))
    {
        auto& child = world.component<transform>(entity);
        auto& parent = world.component<transform>(link.parent);

        if (child.in_scene() != parent.in_scene())
        {
            if (parent.in_scene())
            {
                child.in_scene(true);
                event.publish<event_enter_scene>(entity);
            }
            else
            {
                child.in_scene(false);
                event.publish<event_exit_scene>(entity);
            }
        }
        child.mark_dirty();

        if (world.has_component<bounding_box>(entity))
        {
            auto& bounding = world.component<bounding_box>(entity);
            if (bounding.dynamic())
            {
                std::size_t proxy_id = m_dynamic_bvh.add(bounding.aabb());
                bounding.proxy_id(proxy_id);
            }
            else
            {
                std::size_t proxy_id = m_static_bvh.add(bounding.aabb());
                bounding.proxy_id(proxy_id);
            }
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
        if (node_transform.in_scene())
        {
            node_transform.in_scene(false);
            event.publish<event_exit_scene>(entity);
        }
    }
}
} // namespace ash::scene