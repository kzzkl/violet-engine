#include "rendering/ui_renderer.hpp"
#include "common/log.hpp"
#include <algorithm>

namespace violet
{
static constexpr std::size_t MAX_UI_VERTEX_COUNT = 4096 * 16;
static constexpr std::size_t MAX_UI_INDEX_COUNT = MAX_UI_VERTEX_COUNT * 2;

ui_renderer::ui_renderer(render_device* device)
    : m_draw_list_index(0),
      m_material_parameter_counter(0),
      m_device(device)
{
    /*
    // Position.
    m_vertex_buffers.push_back(graphics::rhi::make_vertex_buffer<float2>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true,
        true));
    // UV.
    m_vertex_buffers.push_back(graphics::rhi::make_vertex_buffer<float2>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true,
        true));
    // Color.
    m_vertex_buffers.push_back(graphics::rhi::make_vertex_buffer<std::uint32_t>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true,
        true));
    // Offset index.
    m_vertex_buffers.push_back(graphics::rhi::make_vertex_buffer<std::uint32_t>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true,
        true));
    m_index_buffer =
        graphics::rhi::make_index_buffer<std::uint32_t>(nullptr, MAX_UI_INDEX_COUNT, true, true);

    m_pipeline = std::make_unique<ui_pipeline>();
    */
}

void ui_renderer::render(widget* root, ui_pass* pass)
{
    auto calculate_overlap_area = [](const widget_extent& a, const widget_extent& b)
    {
        std::uint32_t x1 = std::max(a.x, b.x);
        std::uint32_t x2 = std::min(a.x + a.width, b.x + b.width);
        std::uint32_t y1 = std::max(a.y, b.y);
        std::uint32_t y2 = std::min(a.y + a.height, b.y + b.height);
        return widget_extent{.x = x1, .y = y1, .width = x2 - x1, .height = y2 - y1};
    };

    ui_draw_list* draw_list = allocate_list();
    draw_list->push_scissor(rhi_scissor_rect{
        .min_x = 0,
        .min_y = 0,
        .max_x = root->get_extent().width,
        .max_y = root->get_extent().height});

    render_widget(root, draw_list, root->get_extent());
    draw_list->compile();
    pass->add_draw_list(draw_list);
}

void ui_renderer::reset()
{
    for (std::size_t i = 0; i < m_draw_list_index; ++i)
        m_draw_list_pool[i]->reset();

    m_draw_list_index = 0;
    m_offset.clear();
}

void ui_renderer::render_widget(widget* widget, ui_draw_list* draw_list, widget_extent visible_area)
{
    if (visible_area.width <= 0 || visible_area.height <= 0)
        return;

    widget->paint(draw_list);

    for (auto& child : widget->get_children())
    {
        if (!child->get_visible())
            continue;

        render_widget(child.get(), draw_list, visible_area);
    }
}

/*
void ui_renderer::draw(
    batch_map& batch_map,
    const control_mesh& mesh,
    float x,
    float y,
    float depth)
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
        [vertex_base](std::uint32_t index)
        {
            return index + vertex_base;
        });

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
*/
ui_draw_list* ui_renderer::allocate_list()
{
    ++m_draw_list_index;

    if (m_draw_list_index > m_draw_list_pool.size())
        m_draw_list_pool.push_back(std::make_unique<ui_draw_list>(m_device));

    return m_draw_list_pool[m_draw_list_index - 1].get();
}
} // namespace violet