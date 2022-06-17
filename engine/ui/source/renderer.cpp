#include "ui/renderer.hpp"
#include <algorithm>

namespace ash::ui
{
renderer::renderer() : m_batch_pool_index(0)
{
}

void renderer::draw(element* root)
{
    std::stack<std::pair<element*, std::size_t>> dfs;
    std::stack<batch_map> scissor_stack;

    dfs.push({root, 0});

    while (!dfs.empty())
    {
        auto [node, batch_map_index] = dfs.top();
        dfs.pop();

        if (batch_map_index != scissor_stack.size())
        {
            scissor_stack.pop();
        }

        auto& extent = node->extent();

        auto mesh = node->mesh();
        if (mesh != nullptr)
        {
            if (mesh->scissor)
            {
                batch_map map = {};
                map.scissor = extent;
                scissor_stack.push(map);

                batch_map_index = scissor_stack.size();
            }

            draw(scissor_stack.top(), *mesh, extent.x, extent.y, node->depth());
        }

        for (element* child : node->children())
        {
            if (child->display())
                dfs.push({child, batch_map_index});
        }
    }
}

void renderer::reset()
{
    for (auto& batch : m_batch_pool)
    {
        batch->vertex_position.clear();
        batch->vertex_uv.clear();
        batch->vertex_color.clear();
        batch->vertex_offset_index.clear();
        batch->indices.clear();
    }
    m_batch_pool_index = 0;
    m_offset.clear();
}

void renderer::draw(batch_map& batch_map, const element_mesh& mesh, float x, float y, float depth)
{
    batch_key key = {mesh.type, mesh.texture};
    if (key.type == ELEMENT_MESH_TYPE_IMAGE && key.texture == nullptr)
        return;

    render_batch* target_batch = nullptr;

    auto iter = batch_map.map.find(key);
    if (iter == batch_map.map.end())
    {
        target_batch = allocate_batch();
        target_batch->type = mesh.type;
        target_batch->scissor = batch_map.scissor;
        target_batch->texture = mesh.texture;
        batch_map.map[key] = target_batch;
    }
    else
    {
        target_batch = iter->second;
    }

    std::uint32_t vertex_base = target_batch->vertex_position.size();
    std::uint32_t index_base = target_batch->indices.size();
    target_batch->indices.resize(target_batch->indices.size() + mesh.index_count);
    std::transform(
        mesh.indices,
        mesh.indices + mesh.index_count,
        target_batch->indices.begin() + index_base,
        [vertex_base](std::uint32_t index) { return index + vertex_base; });

    target_batch->vertex_position.insert(
        target_batch->vertex_position.end(),
        mesh.position,
        mesh.position + mesh.vertex_count);
    target_batch->vertex_uv.insert(
        target_batch->vertex_uv.end(),
        mesh.uv,
        mesh.uv + mesh.vertex_count);
    target_batch->vertex_color.insert(
        target_batch->vertex_color.end(),
        mesh.color,
        mesh.color + mesh.vertex_count);
    target_batch->vertex_offset_index.insert(
        target_batch->vertex_offset_index.end(),
        mesh.vertex_count,
        m_offset.size());

    m_offset.push_back({x, y, depth, 1.0f});
}

render_batch* renderer::allocate_batch()
{
    if (m_batch_pool_index >= m_batch_pool.size())
        m_batch_pool.push_back(std::make_unique<render_batch>());

    auto result = m_batch_pool[m_batch_pool_index].get();
    ++m_batch_pool_index;
    return result;
}
} // namespace ash::ui