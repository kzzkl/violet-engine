#include "scene/scene.hpp"
#include "components/transform.hpp"
#include "core/context/engine.hpp"
#include "core/node/node.hpp"

namespace violet::scene
{
scene::scene() : core::engine_module("scene")
{
}

bool scene::initialize(const dictionary& config)
{
    return true;
}

void scene::on_end_frame()
{
    core::view<transform> view(core::engine::get_world());
    view.each([](transform& transform) { transform.reset_update_count(); });
}

void scene::update_transform()
{
    std::queue<core::node*> bfs;

    // Update local matrix.
    core::view<core::node*, transform> view(core::engine::get_world());
    view.each([&bfs](core::node* node, transform& transform) {
        if (node->get_parent() == nullptr)
            bfs.push(node);

        if (transform.get_dirty_flag() & transform::DIRTY_FLAG_LOCAL == 0)
            return;

        math::float4x4_simd local_matrix = math::matrix_simd::affine_transform(
            math::simd::load(transform.get_scale()),
            math::simd::load(transform.get_rotation()),
            math::simd::load(transform.get_position()));
        transform.update_local(local_matrix);
    });

    // Update world matrix.
    while (!bfs.empty())
    {
        core::node* node = bfs.front();
        core::node* parent = node->get_parent();

        bfs.pop();

        auto handle = node->get_component<transform>();
        if (parent == nullptr)
        {
            handle->update_world(handle->get_local_matrix());
        }
        else
        {
            math::float4x4_simd local_matrix = math::simd::load(handle->get_local_matrix());
            math::float4x4_simd parent_world_matrix =
                math::simd::load(parent->get_component<transform>()->get_world_matrix());

            handle->update_world(math::matrix_simd::mul(local_matrix, parent_world_matrix));
        }

        for (core::node* child : node->get_children())
        {
            if (child->has_component<transform>())
                bfs.push(child);
        }
    }
}
} // namespace violet::scene