#include "scene.hpp"

namespace ash::scene
{
scene::scene() : submodule("scene"), m_view(nullptr)
{
}

bool scene::initialize(const dictionary& config)
{
    auto& world = module<ecs::world>();
    world.register_component<transform>();
    m_view = world.make_view<transform>();

    ecs::entity root = world.create();
    world.add<transform>(root);
    m_root = world.component<transform, ecs::read>(root);
    m_root->node->dirty = false;

    return true;
}

void scene::sync_local()
{
    // Update dirty node.
    std::queue<transform_node*> dirty_bfs = find_dirty_node();
    while (!dirty_bfs.empty())
    {
        transform_node* node = dirty_bfs.front();
        dirty_bfs.pop();

        // Update to parent matrix.
        math::float4_simd scale = math::simd::load(node->transform->scaling);
        math::float4_simd rotation = math::simd::load(node->transform->rotation);
        math::float4_simd translation = math::simd::load(node->transform->position);

        math::float4x4_simd to_parent =
            math::matrix_simd::affine_transform(scale, rotation, translation);
        math::simd::store(to_parent, node->transform->parent_matrix);

        math::float4x4_simd parent_to_world =
            math::simd::load(node->parent->transform->world_matrix);
        math::float4x4_simd to_world = math::matrix_simd::mul(to_parent, parent_to_world);
        math::simd::store(to_world, node->transform->world_matrix);

        ++node->sync_count;
        node->dirty = false;

        for (transform_node* child : node->children)
            dirty_bfs.push(child);
    }
}

void scene::sync_world()
{
    // Update to parent matrix.
    std::queue<transform_node*> dirty_bfs = find_dirty_node();
    while (!dirty_bfs.empty())
    {
        transform_node* node = dirty_bfs.front();
        dirty_bfs.pop();

        math::float4x4_simd parent_to_world =
            math::simd::load(node->parent->transform->world_matrix);
        math::float4x4_simd to_world = math::simd::load(node->transform->world_matrix);
        math::float4x4_simd to_parent =
            math::matrix_simd::mul(to_world, math::matrix_simd::inverse(parent_to_world));

        // Update transform data.
        math::float4_simd scaling, rotation, position;
        math::matrix_simd::decompose(to_parent, scaling, rotation, position);

        math::simd::store(to_parent, node->transform->parent_matrix);
        math::simd::store(scaling, node->transform->scaling);
        math::simd::store(rotation, node->transform->rotation);
        math::simd::store(position, node->transform->position);

        ++node->sync_count;
        node->dirty = false;

        for (transform_node* child : node->children)
            dirty_bfs.push(child);
    }
}

void scene::reset_sync_counter()
{
    m_view->each([](transform& t) { t.node->sync_count = 0; });
}

std::queue<transform_node*> scene::find_dirty_node() const
{
    std::queue<transform_node*> check_bfs;
    std::queue<transform_node*> dirty_bfs;

    check_bfs.push(m_root->node.get());
    while (!check_bfs.empty())
    {
        transform_node* node = check_bfs.front();
        check_bfs.pop();

        if (node->dirty)
        {
            dirty_bfs.push(node);
        }
        else
        {
            for (transform_node* child : node->children)
                check_bfs.push(child);
        }
    }

    return dirty_bfs;
}

void scene::link(transform& node)
{
    transform_node& target_node = *node.node;

    // If there is already a parent node, remove from the parent node.
    if (target_node.parent != nullptr)
        unlink(node);

    m_root->node->children.push_back(node.node.get());
    target_node.parent = m_root->node.get();
}

void scene::link(transform& child, transform& parent)
{
    transform_node& parent_node = *parent.node;
    transform_node& child_node = *child.node;

    // If there is already a parent node, remove from the parent node.
    if (child_node.parent != nullptr)
        unlink(child);

    parent_node.children.push_back(child.node.get());
    child_node.parent = parent.node.get();
}

void scene::unlink(transform& node)
{
    transform_node& target_node = *node.node;
    transform_node& parent_node = *target_node.parent;
    for (auto iter = parent_node.children.begin(); iter != parent_node.children.end(); ++iter)
    {
        if (*iter == node.node.get())
        {
            std::swap(*iter, parent_node.children.back());
            parent_node.children.pop_back();
            break;
        }
    }

    target_node.parent = nullptr;
}
} // namespace ash::scene