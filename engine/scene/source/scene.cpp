#include "scene.hpp"

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
    world.component<transform>(m_root).dirty = false;

    return true;
}

void scene::sync_local()
{
    auto& world = system<ecs::world>();

    // Update dirty node.
    std::queue<ecs::entity> dirty_bfs = find_dirty_node();
    while (!dirty_bfs.empty())
    {
        ecs::entity entity = dirty_bfs.front();
        dirty_bfs.pop();

        auto& node = world.component<transform>(entity);
        auto& parent = world.component<transform>(node.parent);

        // Update to parent matrix.
        math::float4_simd scale = math::simd::load(node.scaling);
        math::float4_simd rotation = math::simd::load(node.rotation);
        math::float4_simd translation = math::simd::load(node.position);

        math::float4x4_simd to_parent =
            math::matrix_simd::affine_transform(scale, rotation, translation);
        math::simd::store(to_parent, node.parent_matrix);

        math::float4x4_simd parent_to_world = math::simd::load(parent.world_matrix);
        math::float4x4_simd to_world = math::matrix_simd::mul(to_parent, parent_to_world);
        math::simd::store(to_world, node.world_matrix);

        ++node.sync_count;
        node.dirty = false;

        for (ecs::entity child : node.children)
            dirty_bfs.push(child);
    }
}

void scene::sync_world()
{
    auto& world = system<ecs::world>();

    // Update to parent matrix.
    std::queue<ecs::entity> dirty_bfs = find_dirty_node();
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

std::queue<ecs::entity> scene::find_dirty_node() const
{
    auto& world = system<ecs::world>();

    std::queue<ecs::entity> check_bfs;
    std::queue<ecs::entity> dirty_bfs;

    check_bfs.push(m_root);
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
        unlink(child_entity);

    parent.children.push_back(child_entity);
    child.parent = parent_entity;
}

void scene::unlink(ecs::entity entity)
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
}
} // namespace ash::scene