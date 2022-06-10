#include "ui/controls/dock_window.hpp"
#include "log.hpp"
#include "window/window.hpp"

namespace ash::ui
{
class dock_intermediate_node : public dock_element
{
public:
    void add(dock_element* node);
    void add(dock_element* node, std::size_t index);
    void remove(dock_element* node);

    void update_children_extent(std::size_t index, layout_edge edge, int offset);
    void reset_children_extent();

    virtual bool dockable() const noexcept { return false; }
};

void dock_intermediate_node::add(dock_element* node)
{
    node->link(this);
    reset_children_extent();
}

void dock_intermediate_node::add(dock_element* node, std::size_t index)
{
    node->link(this, index);
    reset_children_extent();
}

void dock_intermediate_node::remove(dock_element* node)
{
    node->unlink();
    reset_children_extent();
}

void dock_intermediate_node::update_children_extent(std::size_t index, layout_edge edge, int offset)
{
    auto& children_node = children();

    if (is_root())
    {
        if (index == 0 && edge == LAYOUT_EDGE_LEFT)
            return;
        if (index == children_node.size() - 1 &&
            (edge == LAYOUT_EDGE_RIGHT || edge == LAYOUT_EDGE_BOTTOM))
            return;
    }

    auto resize_node = static_cast<dock_element*>(children_node[index]);

    if (flex_direction() == LAYOUT_FLEX_DIRECTION_ROW)
    {
        if (edge == LAYOUT_EDGE_LEFT && index != 0)
        {
            float new_width = resize_node->m_width - offset / extent().width * 100.0f;
            resize_node->width_percent(new_width);
            resize_node->m_width = new_width;

            float reserve = 100.0f;
            for (std::size_t i = children_node.size() - 1; i >= index; --i)
                reserve -= static_cast<dock_element*>(children_node[i])->m_width;

            float w = reserve / index;
            for (std::size_t i = 0; i < index; ++i)
            {
                auto child = static_cast<dock_element*>(children_node[i]);
                child->width_percent(w);
                child->m_width = w;
            }
        }
        else if (edge == LAYOUT_EDGE_RIGHT && index != children().size() - 1)
        {
            float new_width = resize_node->m_width + offset / extent().width * 100.0f;
            resize_node->width_percent(new_width);
            resize_node->m_width = new_width;

            float reserve = 100.0f;
            for (std::size_t i = 0; i <= index; ++i)
                reserve -= static_cast<dock_element*>(children_node[i])->m_width;

            float w = reserve / (children_node.size() - index - 1);
            for (std::size_t i = index + 1; i < children_node.size(); ++i)
            {
                auto child = static_cast<dock_element*>(children_node[i]);
                child->width_percent(w);
                child->m_width = w;
            }
        }
        else
        {
            m_dock_node->update_children_extent(link_index(), edge, offset);
        }
    }
    else
    {
        if (edge == LAYOUT_EDGE_BOTTOM && index != children().size() - 1)
        {
            float new_height = resize_node->m_height + offset / extent().height * 100.0f;
            resize_node->height_percent(new_height);
            resize_node->m_height = new_height;

            float reserve = 100.0f;
            for (std::size_t i = 0; i <= index; ++i)
                reserve -= static_cast<dock_element*>(children_node[i])->m_height;

            float h = reserve / (children_node.size() - index - 1);
            for (std::size_t i = index + 1; i < children_node.size(); ++i)
            {
                auto child = static_cast<dock_element*>(children_node[i]);
                child->height_percent(h);
                child->m_height = h;
            }
        }
        else
        {
            m_dock_node->update_children_extent(link_index(), edge, offset);
        }
    }
}

void dock_intermediate_node::reset_children_extent()
{
    if (flex_direction() == LAYOUT_FLEX_DIRECTION_ROW)
    {
        float width_percent = 100.0f / children().size();
        for (auto c : children())
        {
            auto child = static_cast<dock_element*>(c);
            child->width_percent(width_percent);
            child->height_percent(100.0f);
            child->m_width = width_percent;
            child->m_height = 100.0f;
        }
    }
    else
    {
        float height_percent = 100.0f / children().size();
        for (auto c : children())
        {
            auto child = static_cast<dock_element*>(c);
            child->height_percent(height_percent);
            child->width_percent(100.0f);
            child->m_height = height_percent;
            child->m_width = 100.0f;
        }
    }
}

dock_element::dock_element() : m_width(0.0f), m_height(0.0f)
{
}

void dock_element::dock(dock_element* target, layout_edge edge)
{
    undock();

    auto dock_direction = (edge == LAYOUT_EDGE_LEFT || edge == LAYOUT_EDGE_RIGHT)
                              ? LAYOUT_FLEX_DIRECTION_ROW
                              : LAYOUT_FLEX_DIRECTION_COLUMN;

    if (!target->is_root() && dock_direction == target->parent()->flex_direction())
    {
        auto target_parent = static_cast<dock_intermediate_node*>(target->parent());
        if (edge == LAYOUT_EDGE_LEFT || edge == LAYOUT_EDGE_TOP)
            target_parent->add(this, target->link_index());
        else
            target_parent->add(this, target->link_index() + 1);

        ASH_ASSERT(m_dock_node == nullptr);
        m_dock_node = target->m_dock_node;
    }
    else
    {
        target->move_down();
        target->m_dock_node->flex_direction(dock_direction);

        if (edge == LAYOUT_EDGE_LEFT || edge == LAYOUT_EDGE_TOP)
            target->m_dock_node->add(this, 0);
        else
            target->m_dock_node->add(this);

        ASH_ASSERT(m_dock_node == nullptr);
        m_dock_node = target->m_dock_node;
    }
}

void dock_element::undock()
{
    if (is_root())
    {
        unlink();
    }
    else
    {
        m_dock_node->remove(this);

        if (m_dock_node->children().size() == 1)
        {
            dock_element* brother_node = static_cast<dock_element*>(m_dock_node->children()[0]);
            brother_node->move_up();
        }
        m_dock_node = nullptr;
    }
}

void dock_element::move_up()
{
    ASH_ASSERT(m_dock_node != nullptr);
    ASH_ASSERT(m_dock_node->children().size() == 1);

    if (m_dock_node->is_root())
    {
        m_dock_node->remove(this);
        auto& parent_extent = m_dock_node->extent();
        width(parent_extent.width);
        height(parent_extent.height);
        m_width = parent_extent.width;
        m_height = parent_extent.height;

        link(m_dock_node->parent(), m_dock_node->link_index());
        m_dock_node->unlink();
        m_dock_node = nullptr;
    }
    else
    {
        m_dock_node->remove(this);
        m_dock_node->m_dock_node->add(this, m_dock_node->link_index());
        m_dock_node->m_dock_node->remove(m_dock_node.get());
        m_dock_node = m_dock_node->m_dock_node;
    }
}

static std::size_t intermediate_node_counter = 0;
void dock_element::move_down()
{
    auto dock_node = std::make_shared<dock_intermediate_node>();
    dock_node->name = "n" + std::to_string(intermediate_node_counter++);

    if (is_root())
    {
        if (m_width == 0.0f && m_height == 0.0f)
        {
            auto& e = extent();
            dock_node->width(e.width);
            dock_node->height(e.height);
        }
        else
        {
            dock_node->width(m_width);
            dock_node->height(m_height);
        }

        dock_node->link(parent(), link_index());
        unlink();
    }
    else
    {
        m_dock_node->add(dock_node.get(), link_index());
        dock_node->m_dock_node = m_dock_node;
        m_dock_node->remove(this);
    }

    dock_node->add(this);
    m_dock_node = dock_node;
}

void dock_element::resize_percent(float width, float height)
{
    width_percent(width);
    height_percent(height);

    m_width = width;
    m_height = height;
}

dock_panel::dock_panel(std::uint32_t color)
{
    m_mesh.vertex_position = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };
    m_mesh.vertex_uv = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };
    m_mesh.vertex_color = {color, color, color, color};
    m_mesh.indices = {0, 1, 2, 0, 2, 3};
}

void dock_panel::color(std::uint32_t color) noexcept
{
    if (m_mesh.vertex_color[0] != color)
    {
        m_mesh.vertex_color = {color, color, color, color};
        mark_dirty();
    }
}

void dock_panel::render(renderer& renderer)
{
    renderer.draw(RENDER_TYPE_BLOCK, m_mesh);
    element::render(renderer);
}

void dock_panel::on_extent_change()
{
    auto& e = extent();
    float z = depth();
    m_mesh.vertex_position = {
        {e.x,           e.y,            z},
        {e.x + e.width, e.y,            z},
        {e.x + e.width, e.y + e.height, z},
        {e.x,           e.y + e.height, z}
    };
}

dock_window::dock_window(std::string_view title, const dock_window_style& style)
    : dock_panel(style.bar_color),
      m_bar(std::make_unique<element>()),
      m_container(std::make_unique<panel>(style.container_color)),
      m_drag_edge(LAYOUT_EDGE_ALL),
      m_original_position(0)
{
    flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);

    m_bar->align_items(LAYOUT_ALIGN_CENTER);
    m_bar->flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
    m_bar->flex_wrap(LAYOUT_FLEX_WRAP_NOWRAP);
    m_bar->width(1.0f);
    m_bar->link(this);

    label_style title_style = {};
    title_style.text_font = style.title_font;
    title_style.text_color = style.title_color;
    m_title = std::make_unique<label>(title, title_style);
    m_title->link(m_bar.get());

    m_container->width_percent(100.0f);
    m_container->flex_grow(1.0f);
    m_container->link(this);

    m_container->on_mouse_move = [this](int x, int y) {
        layout_edge edge = mouse_over_edge(x, y);
        switch (edge)
        {
        case LAYOUT_EDGE_LEFT:
        case LAYOUT_EDGE_RIGHT: {
            auto& mouse = system<window::window>().mouse();
            mouse.cursor(window::MOUSE_CURSOR_SIZE_WE);
            break;
        }
        case LAYOUT_EDGE_BOTTOM: {
            auto& mouse = system<window::window>().mouse();
            mouse.cursor(window::MOUSE_CURSOR_SIZE_NS);
            break;
        }
        default:
            break;
        }
    };

    m_container->on_mouse_drag_begin = [this](int x, int y) {
        if (is_root())
            return;

        m_drag_edge = mouse_over_edge(x, y);
        switch (m_drag_edge)
        {
        case LAYOUT_EDGE_LEFT:
        case LAYOUT_EDGE_RIGHT:
            m_original_position = x;
            break;
        case LAYOUT_EDGE_BOTTOM:
            m_original_position = y;
            break;
        default:
            break;
        };
    };

    m_container->on_mouse_drag = [this](int x, int y) {
        if (m_drag_edge == LAYOUT_EDGE_ALL)
            return;

        if (m_drag_edge == LAYOUT_EDGE_LEFT || m_drag_edge == LAYOUT_EDGE_RIGHT)
        {
            if (m_original_position != x)
            {
                m_dock_node->update_children_extent(
                    link_index(),
                    m_drag_edge,
                    x - m_original_position);
                m_original_position = x;
            }
        }
        else
        {
            if (m_original_position != y)
            {
                m_dock_node->update_children_extent(
                    link_index(),
                    m_drag_edge,
                    y - m_original_position);
                m_original_position = y;
            }
        }
    };

    m_container->on_mouse_drag_end = [this](int x, int y) { m_drag_edge = LAYOUT_EDGE_ALL; };
}

dock_window::dock_window(
    std::string_view title,
    std::uint32_t icon_index,
    const dock_window_style& style)
    : dock_window(title, style)
{
    m_bar->width(style.icon_font->heigth());

    font_icon_style icon_style = {};
    icon_style.icon_font = style.icon_font;
    icon_style.icon_color = style.icon_color;
    icon_style.icon_scale = style.icon_scale;
    m_icon = std::make_unique<font_icon>(icon_index, icon_style);
    m_icon->margin(style.icon_font->heigth() * style.icon_scale * 0.2f, LAYOUT_EDGE_RIGHT);
    m_icon->link(m_bar.get(), 0);
}

layout_edge dock_window::mouse_over_edge(int mouse_x, int mouse_y) const noexcept
{
    auto& e = extent();
    if (mouse_x - e.x < 10.0f)
        return LAYOUT_EDGE_LEFT;
    else if (e.x + e.width - mouse_x < 10.0f)
        return LAYOUT_EDGE_RIGHT;
    else if (e.y + e.height - mouse_y < 20.0f)
        return LAYOUT_EDGE_BOTTOM;
    else
        return LAYOUT_EDGE_ALL;
}
} // namespace ash::ui