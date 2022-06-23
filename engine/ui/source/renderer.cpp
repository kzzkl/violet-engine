#include "ui/renderer.hpp"
#include "log.hpp"
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
    std::stack<element_extent> visible_area_stack;

    auto calculate_overlap_area = [](const element_extent& a, const element_extent& b) {
        float x1 = std::max(a.x, b.x);
        float x2 = std::min(a.x + a.width, b.x + b.width);
        float y1 = std::max(a.y, b.y);
        float y2 = std::min(a.y + a.height, b.y + b.height);
        return element_extent{.x = x1, .y = y1, .width = x2 - x1, .height = y2 - y1};
    };

    dfs.push({root, 0});
    visible_area_stack.push(root->extent());

    while (!dfs.empty())
    {
        auto [node, batch_map_index] = dfs.top();
        dfs.pop();

        if (batch_map_index != scissor_stack.size())
            scissor_stack.pop();

        auto& extent = node->extent();

        auto mesh = node->mesh();
        if (mesh != nullptr)
        {
            if (mesh->scissor)
            {
                scissor_stack.push(batch_map{.scissor = extent});
                batch_map_index = scissor_stack.size();
            }

            draw(scissor_stack.top(), *mesh, extent.x, extent.y, node->depth());
        }

        element_extent visible_area = visible_area_stack.top();
        visible_area_stack.pop();
        for (element* child : node->children())
        {
            if (!child->display())
                continue;

            element_extent overlap_area = calculate_overlap_area(visible_area, child->extent());
            if (overlap_area.height <= 0 || overlap_area.width <= 0)
                continue;
            else
                visible_area_stack.push(overlap_area);

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
    render_batch* target_batch = nullptr;
    switch (mesh.type)
    {
    case ELEMENT_MESH_TYPE_BLOCK: {
        if (batch_map.block_batch == nullptr)
        {
            target_batch = allocate_batch(ELEMENT_MESH_TYPE_BLOCK, batch_map.scissor, nullptr);
            batch_map.block_batch = target_batch;
        }
        else
        {
            target_batch = batch_map.block_batch;
        }
        break;
    }
    case ELEMENT_MESH_TYPE_TEXT: {
        auto iter = batch_map.text_batch.find(mesh.texture);
        if (iter == batch_map.text_batch.end())
        {
            target_batch = allocate_batch(ELEMENT_MESH_TYPE_TEXT, batch_map.scissor, mesh.texture);
            batch_map.text_batch[mesh.texture] = target_batch;
        }
        else
        {
            target_batch = iter->second;
        }
        break;
    }
    case ELEMENT_MESH_TYPE_IMAGE: {
        auto iter = batch_map.image_batch.find(mesh.texture);
        if (iter == batch_map.image_batch.end())
        {
            target_batch = allocate_batch(ELEMENT_MESH_TYPE_IMAGE, batch_map.scissor, mesh.texture);
            batch_map.image_batch[mesh.texture] = target_batch;
        }
        else
        {
            target_batch = iter->second;
        }
        break;
    }
    default:
        break;
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

render_batch* renderer::allocate_batch(
    element_mesh_type type,
    const element_extent& scissor,
    graphics::resource_interface* texture)
{
    if (m_batch_pool_index >= m_batch_pool.size())
        m_batch_pool.push_back(std::make_unique<render_batch>());

    auto result = m_batch_pool[m_batch_pool_index].get();
    ++m_batch_pool_index;

    result->type = type;
    result->scissor = scissor;
    result->texture = texture;

    return result;
}
} // namespace ash::ui