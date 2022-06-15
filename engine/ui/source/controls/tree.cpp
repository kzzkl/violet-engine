#include "ui/controls/tree.hpp"
#include "ui/controls/font_icon.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"
#include "ui/ui.hpp"
#include "window/window.hpp"

namespace ash::ui
{
static constexpr std::uint32_t CHEVRON_DOWN_INDEX = 0xEA4E;
static constexpr std::uint32_t CHEVRON_UP_INDEX = 0xEA78;

tree_node::tree_node(std::string_view name, const tree_node_theme& theme)
    : m_padding_increment(theme.padding_increment),
      m_child_margin_offset(0.0f),
      m_selected(false),
      m_parent_node(nullptr),
      m_default_color(theme.default_color),
      m_highlight_color(theme.highlight_color)
{
    flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);

    m_top = std::make_unique<panel>(m_default_color);
    m_top->width_percent(100.0f);
    m_top->flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
    m_top->padding(theme.padding_top, LAYOUT_EDGE_TOP);
    m_top->padding(theme.padding_bottom, LAYOUT_EDGE_BOTTOM);
    m_top->padding(0.0f, LAYOUT_EDGE_LEFT);
    m_top->border(theme.text_font->heigth() * 1.5f, LAYOUT_EDGE_HORIZONTAL);
    m_top->align_items(LAYOUT_ALIGN_CENTER);
    m_top->on_mouse_over = [this]() {
        if (!m_selected)
            m_top->color(m_highlight_color);
    };
    m_top->on_mouse_out = [this]() {
        if (!m_selected)
            m_top->color(m_default_color);
    };
    m_top->on_mouse_press = [this](window::mouse_key key, int x, int y) {
        if (key == window::MOUSE_KEY_LEFT && !m_selected)
        {
            m_selected = true;
            m_top->color(m_highlight_color);
            on_select_node(this);
        }

        return false;
    };
    m_top->link(this);

    font_icon_theme icon_theme = {};
    icon_theme.icon_font = &system<ui>().font(DEFAULT_ICON_FONT);
    icon_theme.icon_color = theme.text_color;
    icon_theme.icon_scale = 0.5f;
    m_button = std::make_unique<font_icon>(CHEVRON_DOWN_INDEX, icon_theme);
    m_button->hide();
    m_button->on_mouse_press = [this](window::mouse_key key, int x, int y) {
        if (m_container->children().empty())
            return false;

        if (m_container->display())
        {
            m_button->icon(CHEVRON_DOWN_INDEX);
            m_container->hide();

            if (on_collapse)
                on_collapse();
        }
        else
        {
            m_button->icon(CHEVRON_UP_INDEX);
            m_container->show();

            if (on_expand)
                on_expand();
        }
        return false;
    };
    m_button->width(theme.text_font->heigth() * 1.5f);
    m_button->link(m_top.get());

    label_theme text_theme = {};
    text_theme.text_font = theme.text_font;
    text_theme.text_color = theme.text_color;
    m_text = std::make_unique<label>(name, text_theme);
    m_text->link(m_top.get());

    m_container = std::make_unique<element>();
    m_container->width_percent(100.0f);
    m_container->hide();
    m_container->flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);
    m_container->link(this);
}

tree_node::tree_node(std::string_view name, std::uint32_t icon_index, const tree_node_theme& theme)
    : tree_node(name, theme)
{
    font_icon_theme icon_theme = {};
    icon_theme.icon_font = theme.icon_font;
    icon_theme.icon_color = theme.icon_color;
    icon_theme.icon_scale = theme.icon_scale;
    m_icon = std::make_unique<font_icon>(icon_index, icon_theme);
    m_icon->margin(theme.icon_font->heigth() * theme.icon_scale * 0.5f, LAYOUT_EDGE_RIGHT);
    m_icon->link(m_top.get(), 1);
}

void tree_node::add(tree_node* child)
{
    if (m_container->children().empty())
    {
        m_button->show();
        m_top->border(0.0f, LAYOUT_EDGE_LEFT);
    }

    child->link(m_container.get());
    child->m_top->padding(m_top->padding(LAYOUT_EDGE_LEFT) + m_padding_increment, LAYOUT_EDGE_LEFT);
    child->m_parent_node = this;
}

void tree_node::remove(tree_node* child)
{
    child->unlink();
    child->m_parent_node = nullptr;

    if (m_container->children().empty())
    {
        m_button->hide();
        m_top->border(m_button->extent().width, LAYOUT_EDGE_LEFT);
    }
}

void tree_node::text(std::string_view text)
{
    m_text->text(text);
}

void tree_node::icon(std::uint32_t index)
{
    m_icon->icon(index);
}

void tree_node::icon_scale(float scale)
{
    m_icon->icon_scale(scale);
}

std::string tree_node::text() const
{
    return m_text->text();
}

void tree_node::on_select_node(tree_node* node)
{
    if (m_parent_node)
        m_parent_node->on_select_node(node);
}

tree::tree() : m_selected_node(nullptr)
{
    flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);
}

void tree::add(tree_node* child)
{
    child->link(this);
    child->m_parent_node = this;
}

void tree::on_select_node(tree_node* node)
{
    if (node != m_selected_node)
    {
        if (m_selected_node != nullptr)
        {
            m_selected_node->m_top->color(m_selected_node->m_default_color);
            m_selected_node->m_selected = false;
        }

        m_selected_node = node;

        if (on_select)
            on_select(node);
    }
}
} // namespace ash::ui