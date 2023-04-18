#include "scene/scene.hpp"
#include "components/transform.hpp"
#include "core/context/engine.hpp"
#include "core/node/node.hpp"

namespace violet
{
scene::scene() : engine_module("scene")
{
}

bool scene::initialize(const dictionary& config)
{
    auto& engine_task = engine::get_task_graph();

    engine_task.end_frame.add_task("scene end", []() {
        view<transform> view(engine::get_world());
        view.each([](transform& transform) { transform.reset_update_count(); });
    });

    return true;
}

void scene::update_transform()
{
    std::queue<node*> bfs;

    // Update local matrix.
    view<node*, transform> view(engine::get_world());
    view.each([&bfs](node* node, transform& transform) {
        if (node->get_parent() == nullptr)
            bfs.push(node);

        if (transform.get_dirty_flag() & transform::DIRTY_FLAG_LOCAL == 0)
            return;

        float4x4_simd local_matrix = matrix_simd::affine_transform(
            simd::load(transform.get_scale()),
            simd::load(transform.get_rotation()),
            simd::load(transform.get_position()));
        transform.update_local(local_matrix);
    });

    // Update world matrix.
    while (!bfs.empty())
    {
        node* temp = bfs.front();
        node* parent = temp->get_parent();

        bfs.pop();

        auto handle = temp->get_component<transform>();
        if (parent == nullptr)
        {
            handle->update_world(handle->get_local_matrix());
        }
        else
        {
            float4x4_simd local_matrix = simd::load(handle->get_local_matrix());
            float4x4_simd parent_world_matrix =
                simd::load(parent->get_component<transform>()->get_world_matrix());

            handle->update_world(matrix_simd::mul(local_matrix, parent_world_matrix));
        }

        for (node* child : temp->get_children())
        {
            if (child->has_component<transform>())
                bfs.push(child);
        }
    }
}
} // namespace violet