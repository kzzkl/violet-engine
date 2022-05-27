#include "ui/renderer.hpp"
#include <algorithm>

namespace ash::ui
{
renderer::renderer() : m_batch_pool_index(0)
{
}

void renderer::draw(render_type type, const element_mesh& mesh)
{
    batch_key key = {type, mesh.texture};
    if (key.type == RENDER_TYPE_IMAGE && key.texture == nullptr)
        return;

    auto& current = current_batch_map();
    render_batch* target_batch = nullptr;

    auto iter = current.map.find(key);
    if (iter == current.map.end())
    {
        target_batch = allocate_batch();
        target_batch->type = type;
        target_batch->scissor = current.scissor;
        target_batch->scissor_extent = current.scissor_extent;
        target_batch->texture = mesh.texture;
        current.map[key] = target_batch;
    }
    else
    {
        target_batch = iter->second;
    }

    std::uint32_t vertex_base = target_batch->vertex_position.size();
    std::uint32_t index_base = target_batch->indices.size();
    target_batch->indices.resize(target_batch->indices.size() + mesh.indices.size());
    std::transform(
        mesh.indices.begin(),
        mesh.indices.end(),
        target_batch->indices.begin() + index_base,
        [vertex_base](std::uint32_t index) { return index + vertex_base; });

    target_batch->vertex_position.insert(
        target_batch->vertex_position.end(),
        mesh.vertex_position.begin(),
        mesh.vertex_position.end());
    target_batch->vertex_uv.insert(
        target_batch->vertex_uv.end(),
        mesh.vertex_uv.begin(),
        mesh.vertex_uv.end());
    target_batch->vertex_color.insert(
        target_batch->vertex_color.end(),
        mesh.vertex_color.begin(),
        mesh.vertex_color.end());
}

void renderer::scissor_push(const graphics::scissor_extent& extent)
{
    m_scissor_stack.push(batch_map());
}

void renderer::scissor_pop()
{
    m_scissor_stack.pop();
}

void renderer::reset()
{
    for (auto& batch : m_batch_pool)
    {
        batch.vertex_position.clear();
        batch.vertex_uv.clear();
        batch.vertex_color.clear();
        batch.indices.clear();
    }
    m_batch_pool_index = 0;

    while (!m_scissor_stack.empty())
        m_scissor_stack.pop();
}

render_batch* renderer::allocate_batch()
{
    if (m_batch_pool_index >= m_batch_pool.size())
        m_batch_pool.resize(m_batch_pool_index + 1);

    auto result = &m_batch_pool[m_batch_pool_index];
    ++m_batch_pool_index;
    return result;
}
} // namespace ash::ui