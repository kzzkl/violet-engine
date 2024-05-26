#include "ui/rendering/ui_draw_list.hpp"
#include <algorithm>
#include <iterator>

namespace violet
{
static constexpr std::size_t MAX_UI_VERTEX_COUNT = 4096 * 16;
static constexpr std::size_t MAX_UI_INDEX_COUNT = MAX_UI_VERTEX_COUNT * 2;

ui_draw_list::ui_draw_list(render_device* device) : m_used_batch_count(0), m_device(device)
{
    rhi_buffer_desc vertex_buffer_desc = {};
    vertex_buffer_desc.data = nullptr;
    vertex_buffer_desc.flags = RHI_BUFFER_FLAG_VERTEX | RHI_BUFFER_FLAG_HOST_VISIBLE;

    // position
    vertex_buffer_desc.size = MAX_UI_VERTEX_COUNT * sizeof(float2);
    m_position = m_device->create_buffer(vertex_buffer_desc);

    // uv
    vertex_buffer_desc.size = MAX_UI_VERTEX_COUNT * sizeof(float2);
    m_uv = m_device->create_buffer(vertex_buffer_desc);

    // color
    vertex_buffer_desc.size = MAX_UI_VERTEX_COUNT * sizeof(std::uint32_t);
    m_color = m_device->create_buffer(vertex_buffer_desc);

    // index
    rhi_buffer_desc index_buffer_desc = {};
    index_buffer_desc.data = nullptr;
    index_buffer_desc.flags = RHI_BUFFER_FLAG_INDEX | RHI_BUFFER_FLAG_HOST_VISIBLE;
    index_buffer_desc.size = MAX_UI_INDEX_COUNT * sizeof(std::uint32_t);
    index_buffer_desc.index.size = sizeof(std::uint32_t);

    m_index = m_device->create_buffer(index_buffer_desc);
}

void ui_draw_list::push_scissor(const rhi_scissor_rect& scissor)
{
    ui_draw_group group = {};
    group.scissor = {};
    m_groups.push_back(group);
}

void ui_draw_list::pop_scissor()
{
}

void ui_draw_list::draw_box(float x, float y, float width, float height)
{
    std::vector<float2> position = {
        float2{x,         y         },
        float2{x + width, y         },
        float2{x + width, y + height},
        float2{x,         y + height}
    };

    draw(position, {}, {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}, {0, 1, 2, 0, 2, 3});
}

void ui_draw_list::draw(
    const std::vector<float2>& position,
    const std::vector<float2>& uv,
    const std::vector<std::uint32_t>& color,
    const std::vector<std::uint32_t>& indices,
    rhi_texture* texture)
{
    ui_draw_group& group = m_groups.back();
    ui_draw_batch* batch = nullptr;
    auto iter = group.batches.find(texture);
    if (iter == group.batches.end())
    {
        batch = allocate_batch();
        group.batches[texture] = batch;
    }
    else
    {
        batch = iter->second;
    }

    std::uint32_t vertex_base = batch->position.size();
    std::uint32_t index_base = batch->index.size();
    batch->index.resize(batch->index.size() + indices.size());
    std::transform(
        indices.begin(),
        indices.end(),
        batch->index.begin() + index_base,
        [vertex_base](std::uint32_t index)
        {
            return index + vertex_base;
        });

    batch->position.insert(batch->position.end(), position.begin(), position.end());
    batch->uv.insert(batch->uv.end(), uv.begin(), uv.end());
    batch->color.insert(batch->color.end(), color.begin(), color.end());
}

void ui_draw_list::compile()
{
    float2* position = static_cast<float2*>(m_position->get_buffer());
    float2* uv = static_cast<float2*>(m_uv->get_buffer());
    std::uint32_t* color = static_cast<std::uint32_t*>(m_color->get_buffer());
    std::uint32_t* index = static_cast<std::uint32_t*>(m_index->get_buffer());

    std::size_t vertex_offset = 0;
    std::size_t index_offset = 0;

    for (ui_draw_group& group : m_groups)
    {
        for (auto& [texture, batch] : group.batches)
        {
            std::memcpy(position, batch->position.data(), batch->position.size() * sizeof(float2));
            std::memcpy(uv, batch->uv.data(), batch->uv.size() * sizeof(float2));
            std::memcpy(color, batch->color.data(), batch->color.size() * sizeof(std::uint32_t));
            std::memcpy(index, batch->index.data(), batch->index.size() * sizeof(std::uint32_t));

            m_meshes.push_back(
                {.vertex_start = vertex_offset,
                 .index_start = index_offset,
                 .index_count = batch->index.size(),
                 .texture = texture});

            vertex_offset += batch->position.size();
            index_offset += batch->index.size();
        }
    }
}

void ui_draw_list::reset()
{
    for (std::size_t i = 0; i < m_used_batch_count; ++i)
    {
        m_batches[i]->position.clear();
        m_batches[i]->uv.clear();
        m_batches[i]->color.clear();
        m_batches[i]->index.clear();
    }
    m_used_batch_count = 0;

    m_meshes.clear();
    m_groups.clear();
}

ui_draw_batch* ui_draw_list::allocate_batch()
{
    ++m_used_batch_count;
    if (m_batches.size() < m_used_batch_count)
        m_batches.push_back(std::make_unique<ui_draw_batch>());

    return m_batches[m_used_batch_count - 1].get();
}
} // namespace violet