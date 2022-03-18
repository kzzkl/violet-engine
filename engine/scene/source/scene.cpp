#include "scene.hpp"

namespace ash::scene
{
scene::scene() : submodule("scene"), m_root_node(std::make_unique<scene_node>()), m_view(nullptr)
{
    m_root_node->dirty = false;
    m_root_node->in_scene = true;
}

bool scene::initialize(const dictionary& config)
{
    auto& world = get_submodule<ecs::world>();
    world.register_component<transform>();
    m_view = world.make_view<transform>();

    auto& task = get_submodule<task::task_manager>();
    auto root_task = task.find("root");
    auto scene_task = task.schedule("scene", [this]() {
        update_hierarchy();
        update_to_world();
    });
    scene_task->add_dependency(*root_task);

    return true;
}

void scene::update_hierarchy()
{
    // Update hierarchy and to parent matrix.
    m_view->each([](transform& t) {
        t.node->set_parent(t.parent);
        t.node->in_scene = false;

        math::float4_simd translation = math::simd::load(t.position);
        math::float4_simd rotation = math::simd::load(t.rotation);
        math::float4_simd scaling = math::simd::load(t.scaling);

        math::simd::store(
            math::affine_transform_matrix::make(translation, rotation, scaling),
            t.node->to_parent);
    });
}

void scene::update_to_world()
{
    // Update to world matrix.
    std::queue<scene_node*> bfs;
    bfs.push(m_root_node.get());

    while (!bfs.empty())
    {
        scene_node* node = bfs.front();
        bfs.pop();

        // Update only when there is dirty data
        if (node->dirty)
        {
            math::float4x4_simd to_parent = math::simd::load(node->to_parent);
            math::float4x4_simd parent_to_world = math::simd::load(node->get_parent()->to_world);

            math::float4x4_simd to_world = math::matrix::mul(to_parent, parent_to_world);
            math::simd::store(to_world, node->to_world);

            node->dirty = false;
            node->updated = true;
        }
        else
        {
            node->updated = false;
        }

        for (scene_node* child : node->get_children())
        {
            // When the parent node is updated, the child node also needs to be updated.
            if (node->dirty && !child->dirty)
                child->dirty = true;

            child->in_scene = true;
            bfs.push(child);
        }
    }
}
} // namespace ash::scene