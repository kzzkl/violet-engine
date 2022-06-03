#include "ui/controls/tree.hpp"
#include "ui/ui.hpp"
#include "window/window.hpp"

namespace ash::ui
{
static constexpr std::uint32_t CHEVRON_DOWN_INDEX = 0x0054;
static constexpr std::uint32_t CHEVRON_UP_INDEX = 0x0057;

tree_node::tree_node(std::string_view name, const font& font, const tree_node_style& style)
    : m_padding_increment(style.padding_increment),
      m_selected(false),
      m_parent_node(nullptr),
      m_default_color(style.default_color),
      m_highlight_color(style.highlight_color)
{
    flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);

    m_top = std::make_unique<panel>(m_default_color);
    m_top->resize(100.0f, font.heigth(), false, true, true, false);
    m_top->flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
    m_top->padding(style.padding_top, LAYOUT_EDGE_TOP);
    m_top->padding(style.padding_bottom, LAYOUT_EDGE_BOTTOM);
    m_top->padding(0.0f, LAYOUT_EDGE_LEFT);
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

    m_button =
        std::make_unique<font_icon>(CHEVRON_DOWN_INDEX, system<ui>().font("dripicons-v2"), 0.6f);
    m_button->hide();
    m_button->on_mouse_press = [this](window::mouse_key key, int x, int y) {
        if (m_container->children().empty())
            return false;

        if (m_container->display())
        {
            m_button->reset(CHEVRON_DOWN_INDEX, system<ui>().font("dripicons-v2"), 0.6f);
            m_container->hide();
        }
        else
        {
            m_button->reset(CHEVRON_UP_INDEX, system<ui>().font("dripicons-v2"), 0.6f);
            m_container->show();
        }
        return false;
    };
    m_button->resize(font.heigth() * 1.5f, font.heigth());
    m_button->link(m_top.get());

    m_name = std::make_unique<label>(name, font, style.font_color);
    m_name->margin(font.heigth() * 1.5f, LAYOUT_EDGE_LEFT);
    m_name->link(m_top.get());

    m_container = std::make_unique<element>();
    m_container->resize(100.0f, 0.0f, false, true, true, false);
    m_container->hide();
    m_container->flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);
    m_container->link(this);
}

void tree_node::add(tree_node* child)
{
    if (m_container->children().empty())
    {
        m_button->show();
        m_name->margin(0.0f, LAYOUT_EDGE_LEFT);
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
        m_name->margin(m_button->extent().width, LAYOUT_EDGE_LEFT);
    }
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