#include "ui/widgets/input.hpp"
#include "common/log.hpp"
#include "ui/font.hpp"
#include <format>

namespace violet
{
class text_input : public control
{
public:
    text_input(const font* font, std::uint32_t color);

    void text(std::string_view content);
    void text_color(std::uint32_t color);

    void select(int begin, int end) noexcept;
    void select_all();
    std::pair<int, int> select_range() const noexcept { return {m_select_begin, m_select_end}; }

    std::string text() const noexcept { return m_text; }
    virtual const control_mesh* mesh() const noexcept override { return &m_mesh; }

public:
    using on_select_text_event = control_event<void(int, int)>;

    on_select_text_event::handle on_select_text;

private:
    int find_char(int x, int y);

    std::string m_text;
    const font* m_font;

    std::vector<float2> m_position;
    std::vector<float2> m_uv;
    std::vector<std::uint32_t> m_color;
    std::vector<std::uint32_t> m_indices;

    control_mesh m_mesh;

    int m_select_begin;
    int m_select_end;

    int m_drag_x;
};

text_input::text_input(const font* font, std::uint32_t color) : m_font(font)
{
    text("");
    text_color(color);

    event()->on_mouse_drag_begin = [this](int x, int y) {
        auto& e = extent();
        m_select_begin = m_select_end = find_char(x - e.x, y - e.y);
        m_drag_x = x;

        if (on_select_text)
            on_select_text(m_position[m_select_begin * 4][0], m_position[m_select_end * 4][0]);
    };

    event()->on_mouse_drag = [this](int x, int y) {
        if (x == m_drag_x)
            return;
        m_drag_x = x;

        auto& e = extent();
        int end = find_char(x - e.x, y - e.y);
        if (end == m_select_begin)
            return;

        if (end != m_select_end && on_select_text)
        {
            m_select_end = end;
            if (m_select_begin < m_select_end)
                on_select_text(m_position[m_select_begin * 4][0], m_position[m_select_end * 4][0]);
            else
                on_select_text(m_position[m_select_end * 4][0], m_position[m_select_begin * 4][0]);
        }
    };

    m_mesh.type = ELEMENT_MESH_TYPE_TEXT;
}

void text_input::text(std::string_view content)
{
    m_position.clear();
    m_uv.clear();
    m_indices.clear();

    float pen_x = 0.0f;
    float pen_y = m_font->heigth() * 0.75f;

    std::uint32_t vertex_base = 0;
    auto add_char = [&](char c) {
        auto& glyph = m_font->glyph(c);

        float x = pen_x + glyph.bearing_x;
        float y = pen_y - glyph.bearing_y;

        m_position.insert(
            m_position.end(),
            {
                {x,               y               },
                {x + glyph.width, y               },
                {x + glyph.width, y + glyph.height},
                {x,               y + glyph.height}
        });

        m_uv.insert(
            m_uv.end(),
            {
                glyph.uv1,
                {glyph.uv2[0], glyph.uv1[1]},
                glyph.uv2,
                {glyph.uv1[0], glyph.uv2[1]}
        });
        m_indices.insert(
            m_indices.end(),
            {vertex_base,
             vertex_base + 1,
             vertex_base + 2,
             vertex_base,
             vertex_base + 2,
             vertex_base + 3});

        vertex_base += 4;
        pen_x += glyph.advance;
    };

    for (char c : content)
        add_char(c);
    add_char(' ');

    if (m_color.empty())
        m_color.resize(m_position.size());
    else
        m_color.resize(m_position.size(), m_color[0]);

    m_mesh.position = m_position.data();
    m_mesh.uv = m_uv.data();
    m_mesh.color = m_color.data();
    m_mesh.vertex_count = m_position.size();
    m_mesh.indices = m_indices.data();
    m_mesh.index_count = m_indices.size();
    m_mesh.texture = m_font->texture();

    layout()->set_width(pen_x);
    layout()->set_height(m_font->heigth());

    m_text = content;

    mark_dirty();
}

void text_input::text_color(std::uint32_t color)
{
    for (auto& c : m_color)
        c = color;

    mark_dirty();
}

void text_input::select(int begin, int end) noexcept
{
    m_select_begin = begin;
    m_select_end = end;

    if (on_select_text)
        on_select_text(m_position[m_select_begin * 4][0], m_position[m_select_end * 4][0]);
}

void text_input::select_all()
{
    m_select_begin = 0;
    m_select_end = m_text.size();

    if (on_select_text)
        on_select_text(m_position[m_select_begin * 4][0], m_position[m_select_end * 4][0]);
}

int text_input::find_char(int x, int y)
{
    int left = 0;
    int right = m_text.size();

    while (left < right)
    {
        int mid = (left + right) / 2;

        if (x < m_position[mid * 4][0])
            right = mid - 1;
        else if (m_position[(mid + 1) * 4][0] < x)
            left = mid + 1;
        else
            return mid;
    }
    return left;
}

input::input(const input_theme& theme)
    : panel(theme.background_color),
      m_text_change(false),
      m_background_color(theme.background_color),
      m_underline_color(theme.underline_color)
{
    m_text = std::make_unique<text_input>(theme.text_font, theme.text_color);
    m_text->layer(2);
    m_text->layout()->set_margin(10.0f, LAYOUT_EDGE_HORIZONTAL);
    m_text->on_select_text = [this](int left, int right) {
        auto& e = extent();
        auto& text_extent = m_text->extent();
        m_select->layout()->set_position(text_extent.x - e.x + left - 1.0f, LAYOUT_EDGE_LEFT);
        if (left == right)
        {
            m_select->layout()->set_width(2.0f);
        }
        else
        {
            m_select->layout()->set_width(right - left + 2.0f);
        }
    };

    add(m_text.get());

    m_select = std::make_unique<panel>(theme.select_color);
    m_select->layout()->set_position_type(LAYOUT_POSITION_TYPE_ABSOLUTE);
    m_select->layout()->set_height_percent(100.0f);
    m_select->hide();
    add(m_select.get());

    m_underline = std::make_unique<panel>(theme.underline_color);
    m_underline->layout()->set_position_type(LAYOUT_POSITION_TYPE_ABSOLUTE);
    m_underline->layout()->set_position(0.0f, LAYOUT_EDGE_BOTTOM);
    m_underline->layout()->set_height(2.0f);
    m_underline->layout()->set_width_percent(100.0f);
    m_underline->hide();
    add(m_underline.get());

    event()->on_focus = [this]() {
        m_select->show();
        m_underline->show();
        m_text->select_all();
    };
    event()->on_blur = [this]() {
        m_select->hide();
        m_underline->hide();
        if (m_text_change)
        {
            if (!set_value(m_text->text()))
                m_text->text(adjust_text());

            if (on_text_change)
                on_text_change(m_text->text());

            m_text_change = false;
        }
    };
    event()->on_input = [this](char c) {
        put_char(c);
        m_text_change = true;
    };
}

void input::text(std::string_view content)
{
    m_text->text(content);
}

std::string input::text() const noexcept
{
    return m_text->text();
}

void input::put_char(char c)
{
    std::string new_text = m_text->text();
    auto [select_begin, select_end] = m_text->select_range();

    if (select_begin != select_end)
    {
        if (select_begin < select_end)
        {
            new_text.erase(select_begin, select_end - select_begin);
        }
        else
        {
            new_text.erase(select_end, select_begin - select_end);
            std::swap(select_begin, select_end);
        }
    }

    if (c == 0x08)
    {
        // backspace
        if (select_begin == select_end && select_begin > 0)
        {
            new_text.erase(select_begin - 1, 1);
            --select_begin;
        }
    }
    else if (0x20 <= c && c <= 0x7E)
    {
        new_text.insert(new_text.begin() + select_begin, c);
        ++select_begin;
    }
    else
    {
        return;
    }

    m_text->text(new_text);
    m_text->select(select_begin, select_begin);
}

input_float::input_float(const input_theme& theme) : input(theme), m_value(0.0f)
{
    text(std::format("{:.5f}", m_value));
}

void input_float::value(float value)
{
    m_value = value;
    text(std::format("{:.5f}", m_value));
}

bool input_float::set_value(std::string_view text)
{
    try
    {
        m_value = std::stof(text.data());
    }
    catch (...)
    {
        return false;
    }

    if (on_value_change)
        on_value_change(m_value);

    return false;
}

std::string input_float::adjust_text() const noexcept
{
    return std::format("{}", m_value);
}
} // namespace violet