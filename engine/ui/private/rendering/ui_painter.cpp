#include "ui/rendering/ui_painter.hpp"
#include <algorithm>
#include <iterator>

namespace violet
{
static constexpr std::size_t MAX_UI_VERTEX_COUNT = 4096 * 16;
static constexpr std::size_t MAX_UI_INDEX_COUNT = MAX_UI_VERTEX_COUNT * 2;

ui_painter::ui_painter(font* default_font, render_device* device)
    : m_default_font(default_font),
      m_device(device)
{
    m_mvp_parameter = device->create_parameter(get_mvp_parameter_layout());
    m_sampler = device->create_sampler({});

    rhi_buffer_desc vertex_buffer_desc = {};
    vertex_buffer_desc.data = nullptr;
    vertex_buffer_desc.flags = RHI_BUFFER_FLAG_VERTEX | RHI_BUFFER_FLAG_HOST_VISIBLE;

    // position
    vertex_buffer_desc.size = MAX_UI_VERTEX_COUNT * sizeof(float2);
    m_position_buffer = m_device->create_buffer(vertex_buffer_desc);

    // uv
    vertex_buffer_desc.size = MAX_UI_VERTEX_COUNT * sizeof(float2);
    m_uv_buffer = m_device->create_buffer(vertex_buffer_desc);

    // color
    vertex_buffer_desc.size = MAX_UI_VERTEX_COUNT * sizeof(std::uint32_t);
    m_color_buffer = m_device->create_buffer(vertex_buffer_desc);

    // index
    rhi_buffer_desc index_buffer_desc = {};
    index_buffer_desc.data = nullptr;
    index_buffer_desc.flags = RHI_BUFFER_FLAG_INDEX | RHI_BUFFER_FLAG_HOST_VISIBLE;
    index_buffer_desc.size = MAX_UI_INDEX_COUNT * sizeof(std::uint32_t);
    index_buffer_desc.index.size = sizeof(std::uint32_t);

    m_index_buffer = m_device->create_buffer(index_buffer_desc);
}

void ui_painter::push_group(bool scissor, const rhi_scissor_rect& scissor_rect)
{
    ui_draw_group* group = allocate_group();
    group->scissor = scissor;
    group->scissor_rect = scissor_rect;
    m_group_stack.push(group);
}

void ui_painter::pop_group()
{
    m_group_stack.pop();
}

void ui_painter::draw_rect(float width, float height)
{
    std::vector<float2> position = {
        m_pen_position,
        float2{m_pen_position[0] + width, m_pen_position[1]         },
        float2{m_pen_position[0] + width, m_pen_position[1] + height},
        float2{m_pen_position[0],         m_pen_position[1] + height}
    };

    std::vector<float2> uv(4);
    std::vector<std::uint32_t> color(4, m_pen_color);

    draw_mesh(position, color, uv, {0, 2, 1, 0, 3, 2});
}

void ui_painter::draw_text(std::string_view text, font* font)
{
    if (font == nullptr)
        font = m_default_font;

    std::vector<float2> position;
    std::vector<float2> uv;
    std::vector<std::uint32_t> colors;
    std::vector<std::uint32_t> indices;

    float pen_x = m_pen_position[0];
    float pen_y = m_pen_position[1] + static_cast<std::uint32_t>(font->get_heigth() * 0.75f);

    std::uint32_t vertex_base = 0;
    for (char c : text)
    {
        auto& glyph = font->get_glyph(c);

        float x = pen_x + glyph.bearing_x;
        float y = pen_y - glyph.bearing_y;

        position.push_back({x, y});
        position.push_back({x + glyph.width, y});
        position.push_back({x + glyph.width, y + glyph.height});
        position.push_back({x, y + glyph.height});

        uv.push_back(glyph.uv1);
        uv.push_back({glyph.uv2[0], glyph.uv1[1]});
        uv.push_back(glyph.uv2);
        uv.push_back({glyph.uv1[0], glyph.uv2[1]});

        indices.push_back(vertex_base);
        indices.push_back(vertex_base + 1);
        indices.push_back(vertex_base + 2);
        indices.push_back(vertex_base);
        indices.push_back(vertex_base + 2);
        indices.push_back(vertex_base + 3);

        vertex_base += 4;
        pen_x += glyph.advance;
    }

    colors.resize(position.size(), m_pen_color);

    draw_mesh(position, colors, uv, indices, font->get_texture());
}

void ui_painter::draw_line(const float2& start, const float2& end, float thickness)
{
    draw_path({start, end}, thickness);
}

void ui_painter::draw_path(const std::vector<float2>& points, float thickness)
{
    if (points.size() < 2)
        return;

    float half_thickness = thickness * 0.5f;

    std::vector<float2> position(points.size() * 2);
    std::vector<std::uint32_t> colors(position.size(), m_pen_color);
    std::vector<float2> uv(position.size());
    std::vector<std::uint32_t> indices((points.size() - 1) * 6);

    std::vector<float2> normals(points.size());
    for (std::size_t i = 0; i < points.size() - 1; ++i)
    {
        vector4 n = vector::normalize(
            vector::set(points[i].y - points[i + 1].y, points[i + 1].x - points[i].x));

        math::store(n, normals[i]);
    }
    normals[normals.size() - 1] = normals[normals.size() - 2];

    for (std::size_t i = 0; i < points.size(); ++i)
    {
        std::size_t prev = i == 0 ? 0 : i - 1;

        vector4 prev_normal = math::load(normals[prev]);
        vector4 normal = math::load(normals[i]);

        normal = vector::add(prev_normal, normal);
        normal = vector::mul(normal, 0.5f);
        float d2 = vector::dot(normal, normal);

        if (d2 > 0.000001f)
        {
            float inv_len2 = std::min(1.0f / d2, 100.0f);
            normal = vector::mul(normal, inv_len2);
        }
        normal = vector::mul(normal, half_thickness);

        vector4 point = math::load(points[i]);

        math::store(vector::add(point, normal), position[i * 2]);
        math::store(vector::sub(point, normal), position[i * 2 + 1]);
    }

    for (std::size_t i = 0; i < points.size() - 1; ++i)
    {
        indices[i * 6 + 0] = 0 + i * 2;
        indices[i * 6 + 1] = 3 + i * 2;
        indices[i * 6 + 2] = 2 + i * 2;
        indices[i * 6 + 3] = 0 + i * 2;
        indices[i * 6 + 4] = 1 + i * 2;
        indices[i * 6 + 5] = 3 + i * 2;
    }

    draw_mesh(position, colors, uv, indices);
}

void ui_painter::draw_mesh(
    const std::vector<float2>& position,
    const std::vector<std::uint32_t>& color,
    const std::vector<float2>& uv,
    const std::vector<std::uint32_t>& indices,
    rhi_texture* texture)
{
    ui_draw_group& group = *m_group_stack.top();
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
    batch->color.insert(batch->color.end(), color.begin(), color.end());
    batch->uv.insert(batch->uv.end(), uv.begin(), uv.end());
}

void ui_painter::set_extent(std::uint32_t width, std::uint32_t height)
{
    float4x4 orthographic = math::store<float4x4>(matrix::orthographic(
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        0.0f,
        0.0f,
        1.0f));

    m_mvp_parameter->set_uniform(0, &orthographic, sizeof(float4x4), 0);
}

std::uint32_t ui_painter::get_text_width(std::string_view text, font* font)
{
    if (font == nullptr)
        font = m_default_font;

    return font->get_width(text);
}

std::uint32_t ui_painter::get_text_height(std::string_view text, font* font)
{
    if (font == nullptr)
        font = m_default_font;

    return font->get_heigth();
}

void ui_painter::compile()
{
    float2* position = static_cast<float2*>(m_position_buffer->get_buffer());
    std::uint32_t* color = static_cast<std::uint32_t*>(m_color_buffer->get_buffer());
    float2* uv = static_cast<float2*>(m_uv_buffer->get_buffer());
    std::uint32_t* index = static_cast<std::uint32_t*>(m_index_buffer->get_buffer());

    std::size_t vertex_offset = 0;
    std::size_t index_offset = 0;

    for (std::size_t i = 0; i < m_group_count; ++i)
    {
        for (auto& [texture, batch] : m_group_pool[i]->batches)
        {
            std::memcpy(
                position + vertex_offset,
                batch->position.data(),
                batch->position.size() * sizeof(float2));
            std::memcpy(
                color + vertex_offset,
                batch->color.data(),
                batch->color.size() * sizeof(std::uint32_t));
            std::memcpy(uv + vertex_offset, batch->uv.data(), batch->uv.size() * sizeof(float2));
            std::memcpy(
                index + index_offset,
                batch->index.data(),
                batch->index.size() * sizeof(std::uint32_t));

            rhi_parameter* parameter = allocate_parameter();
            if (texture == nullptr)
            {
                std::uint32_t type = 0;
                parameter->set_uniform(0, &type, sizeof(std::uint32_t), 0);
                parameter->set_texture(1, m_default_font->get_texture(), m_sampler.get());
            }
            else
            {
                std::uint32_t type = 1;
                parameter->set_uniform(0, &type, sizeof(std::uint32_t), 0);
                parameter->set_texture(1, texture, m_sampler.get());
            }

            m_group_pool[i]->meshes.push_back(
                {.vertex_start = vertex_offset,
                 .index_start = index_offset,
                 .index_count = batch->index.size(),
                 .parameter = parameter});

            vertex_offset += batch->position.size();
            index_offset += batch->index.size();
        }
    }
}

void ui_painter::reset()
{
    for (std::size_t i = 0; i < m_batch_count; ++i)
    {
        m_batche_pool[i]->position.clear();
        m_batche_pool[i]->color.clear();
        m_batche_pool[i]->uv.clear();
        m_batche_pool[i]->index.clear();
    }
    m_batch_count = 0;
    m_parameter_count = 0;

    while (!m_group_stack.empty())
        m_group_stack.pop();

    for (std::size_t i = 0; i < m_group_count; ++i)
    {
        m_group_pool[i]->batches.clear();
        m_group_pool[i]->meshes.clear();
    }
    m_group_count = 0;
}

ui_draw_group* ui_painter::allocate_group()
{
    ++m_group_count;
    if (m_group_pool.size() < m_group_count)
        m_group_pool.push_back(std::make_unique<ui_draw_group>());

    return m_group_pool[m_group_count - 1].get();
}

ui_draw_batch* ui_painter::allocate_batch()
{
    ++m_batch_count;
    if (m_batche_pool.size() < m_batch_count)
        m_batche_pool.push_back(std::make_unique<ui_draw_batch>());

    return m_batche_pool[m_batch_count - 1].get();
}

rhi_parameter* ui_painter::allocate_parameter()
{
    ++m_parameter_count;
    if (m_parameter_pool.size() < m_parameter_count)
        m_parameter_pool.push_back(m_device->create_parameter(get_material_parameter_layout()));

    return m_parameter_pool[m_parameter_count - 1].get();
}
} // namespace violet