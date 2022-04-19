#include "scene.hpp"
#include "scene_event.hpp"

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

    m_root = world.create();
    world.add<transform>(m_root);

    auto& root_transform = world.component<transform>(m_root);
    root_transform.dirty = false;
    root_transform.in_scene = true;

    auto& event = system<core::event>();
    event.register_event<event_enter_scene>();
    event.register_event<event_exit_scene>();

    return true;
}

void scene::sync_local()
{
    sync_local(m_root);
}

void scene::sync_local(ecs::entity root)
{
    auto& world = system<ecs::world>();

    // Update dirty node.
    std::queue<ecs::entity> dirty_bfs = find_dirty_node(root);
    while (!dirty_bfs.empty())
    {
        ecs::entity entity = dirty_bfs.front();
        dirty_bfs.pop();

        auto& node = world.component<transform>(entity);

        // Update to parent matrix.
        math::float4_simd scale = math::simd::load(node.scaling);
        math::float4_simd rotation = math::simd::load(node.rotation);
        math::float4_simd translation = math::simd::load(node.position);

        math::float4x4_simd to_parent =
            math::matrix_simd::affine_transform(scale, rotation, translation);
        math::simd::store(to_parent, node.parent_matrix);

        // Update to world matrix.
        if (node.parent != ecs::INVALID_ENTITY)
        {
            auto& parent = world.component<transform>(node.parent);
            math::float4x4_simd parent_to_world = math::simd::load(parent.world_matrix);
            math::float4x4_simd to_world = math::matrix_simd::mul(to_parent, parent_to_world);
            math::simd::store(to_world, node.world_matrix);
        }
        else
        {
            node.world_matrix = node.parent_matrix;
        }

        ++node.sync_count;
        node.dirty = false;

        for (ecs::entity child : node.children)
            dirty_bfs.push(child);
    }
}

void scene::sync_world()
{
    sync_world(m_root);
}

void scene::sync_world(ecs::entity root)
{
    auto& world = system<ecs::world>();

    // Update to parent matrix.
    std::queue<ecs::entity> dirty_bfs = find_dirty_node(root);
    while (!dirty_bfs.empty())
    {
        ecs::entity entity = dirty_bfs.front();
        dirty_bfs.pop();

        auto& node = world.component<transform>(entity);
        auto& parent = world.component<transform>(node.parent);

        math::float4x4_simd parent_to_world = math::simd::load(parent.world_matrix);
        math::float4x4_simd to_world = math::simd::load(node.world_matrix);
        math::float4x4_simd to_parent =
            math::matrix_simd::mul(to_world, math::matrix_simd::inverse(parent_to_world));

        // Update transform data.
        math::float4_simd scaling, rotation, position;
        math::matrix_simd::decompose(to_parent, scaling, rotation, position);

        math::simd::store(to_parent, node.parent_matrix);
        math::simd::store(scaling, node.scaling);
        math::simd::store(rotation, node.rotation);
        math::simd::store(position, node.position);

        ++node.sync_count;
        node.dirty = false;

        for (ecs::entity child : node.children)
            dirty_bfs.push(child);
    }
}

void scene::reset_sync_counter()
{
    m_view->each([](transform& transform) { transform.sync_count = 0; });
}

std::queue<ecs::entity> scene::find_dirty_node(ecs::entity root) const
{
    auto& world = system<ecs::world>();

    std::queue<ecs::entity> check_bfs;
    std::queue<ecs::entity> dirty_bfs;

    check_bfs.push(root);
    while (!check_bfs.empty())
    {
        ecs::entity entity = check_bfs.front();
        check_bfs.pop();

        auto& node = world.component<transform>(entity);
        if (node.dirty)
        {
            dirty_bfs.push(entity);
        }
        else
        {
            for (ecs::entity child : node.children)
                check_bfs.push(child);
        }
    }

    return dirty_bfs;
}

void scene::link(ecs::entity entity)
{
    link(entity, m_root);
}

void scene::link(ecs::entity child_entity, ecs::entity parent_entity)
{
    auto& world = system<ecs::world>();

    auto& child = world.component<transform>(child_entity);
    auto& parent = world.component<transform>(parent_entity);

    // If there is already a parent node, remove from the parent node.
    if (child.parent != ecs::INVALID_ENTITY)
        unlink(child_entity, false);

    parent.children.push_back(child_entity);
    child.parent = parent_entity;

    if (child.in_scene && !parent.in_scene)
    {
        // Exit scene.
        each_children(child_entity, [&, this](ecs::entity node) {
            world.component<transform>(node).in_scene = false;
        });
        system<core::event>().publish<event_exit_scene>(child_entity);
    }
    else if (!child.in_scene && parent.in_scene)
    {
        // Enter scene.
        each_children(child_entity, [&, this](ecs::entity node) {
            world.component<transform>(node).in_scene = true;
        });
        system<core::event>().publish<event_enter_scene>(child_entity);
    }
}

void scene::unlink(ecs::entity entity, bool send_event)
{
    auto& world = system<ecs::world>();

    auto& child = world.component<transform>(entity);
    auto& parent = world.component<transform>(child.parent);

    for (auto iter = parent.children.begin(); iter != parent.children.end(); ++iter)
    {
        if (*iter == entity)
        {
            std::swap(*iter, parent.children.back());
            parent.children.pop_back();
            break;
        }
    }

    child.parent = ecs::INVALID_ENTITY;

    if (parent.in_scene && send_event)
        system<core::event>().publish<event_exit_scene>(entity);
}
} // namespace ash::scene