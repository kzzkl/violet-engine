#include "scene.hpp"

namespace ash::scene
{
scene::scene()
    : submodule("scene"),
      m_root_node(std::make_unique<scene_node>(nullptr)),
      m_view(nullptr)
{
    m_root_node->in_scene(true);
}

bool scene::initialize(const dictionary& config)
{
    auto& world = module<ecs::world>();
    world.register_component<transform>();
    m_view = world.make_view<transform>();

    return true;
}

void scene::sync_local()
{
    // Update dirty node.
    std::queue<scene_node*> dirty_bfs = find_dirty_node();
    while (!dirty_bfs.empty())
    {
        scene_node* node = dirty_bfs.front();
        dirty_bfs.pop();

        // Update to parent matrix.
        math::float4_simd scale = math::simd::load(node->transform()->scaling());
        math::float4_simd rotation = math::simd::load(node->transform()->rotation());
        math::float4_simd translation = math::simd::load(node->transform()->position());

        math::float4x4_simd to_parent =
            math::matrix_simd::affine_transform(scale, rotation, translation);
        math::simd::store(to_parent, node->to_parent);

        math::float4x4_simd parent_to_world = math::simd::load(node->parent()->to_world);
        math::float4x4_simd to_world = math::matrix_simd::mul(to_parent, parent_to_world);
        math::simd::store(to_world, node->to_world);

        node->mark_sync();

        for (scene_node* child : node->children())
            dirty_bfs.push(child);
    }
}

void scene::sync_world()
{
    // Update to parent matrix.
    std::queue<scene_node*> dirty_bfs = find_dirty_node();
    while (!dirty_bfs.empty())
    {
        scene_node* node = dirty_bfs.front();
        dirty_bfs.pop();

        math::float4x4_simd parent_to_world = math::simd::load(node->parent()->to_world);
        math::float4x4_simd to_world = math::simd::load(node->to_world);
        math::float4x4_simd to_parent =
            math::matrix_simd::mul(to_world, math::matrix_simd::inverse(parent_to_world));

        // Update transform data.
        math::float4_simd scaling, rotation, position;
        math::matrix_simd::decompose(to_parent, scaling, rotation, position);

        math::simd::store(to_parent, node->to_parent);
        node->transform()->scaling(scaling);
        node->transform()->rotation(rotation);
        node->transform()->position(position);

        node->mark_sync();

        for (scene_node* child : node->children())
            dirty_bfs.push(child);
    }
}

void scene::reset_sync_counter()
{
    m_view->each([](transform& t) { t.node()->reset_sync_count(); });
}

std::queue<scene_node*> scene::find_dirty_node() const
{
    std::queue<scene_node*> check_bfs;
    std::queue<scene_node*> dirty_bfs;

    check_bfs.push(m_root_node.get());
    while (!check_bfs.empty())
    {
        scene_node* node = check_bfs.front();
        check_bfs.pop();

        if (node->dirty())
        {
            dirty_bfs.push(node);
        }
        else
        {
            for (scene_node* child : node->children())
                check_bfs.push(child);
        }
    }

    return dirty_bfs;
}
} // namespace ash::scene