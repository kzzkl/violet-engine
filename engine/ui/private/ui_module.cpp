#include "ui/ui_module.hpp"
#include "components/camera.hpp"
#include "components/ui_root.hpp"
#include "graphics/graphics_module.hpp"
#include "rendering/ui_renderer.hpp"
#include "window/window_module.hpp"
#include "common/log.hpp"

namespace violet
{
ui_module::ui_module() : engine_module("ui")
{
}

bool ui_module::initialize(const dictionary& config)
{
    get_world().register_component<ui_root>();

    auto& window = get_module<window_module>();

    load_font("remixicon", "engine/font/remixicon.ttf", 24);
    load_font("NotoSans-Regular", "engine/font/NotoSans-Regular.ttf", 13);

    m_renderer = std::make_unique<ui_renderer>(get_module<graphics_module>().get_device());
    m_renderer->set_default_font(get_default_text_font());

    on_tick().then(
        [this](float delta)
        {
            tick();
        });

    return true;
}

void ui_module::tick()
{
    m_renderer->reset();

    update_input();

    view<ui_root, camera> view(get_world());
    view.each(
        [this](ui_root& root, camera& camera)
        {
            rhi_viewport viewport = camera.get_viewport();
            widget* container = root.get_container();
            update_layout(container, viewport.width, viewport.height);
            render(container, root.get_pass());
        });
}

void ui_module::load_font(std::string_view name, std::string_view ttf_file, std::size_t size)
{
    m_fonts[name.data()] =
        std::make_unique<font>(ttf_file, size, get_module<graphics_module>().get_device());
}

font* ui_module::get_font(std::string_view name)
{
    auto iter = m_fonts.find(name.data());
    if (iter != m_fonts.end())
        return iter->second.get();
    else
        throw std::out_of_range("Font not found.");
}

font* ui_module::get_default_text_font()
{
    return get_font("NotoSans-Regular");
}

font* ui_module::get_default_icon_font()
{
    return get_font("remixicon");
}

void ui_module::update_input()
{
    auto& mouse = get_module<window_module>().get_mouse();
    if (mouse.get_mode() == MOUSE_MODE_RELATIVE)
        return;

    auto in_extent = [](int x, int y, const widget_extent& extent) -> bool
    {
        return extent.x <= x && extent.x + extent.width >= x && extent.y <= y &&
               extent.y + extent.height >= y;
    };

    int mouse_x = mouse.get_x();
    int mouse_y = mouse.get_y();

    for (auto widget : m_mouse_over_widgets)
    {
        if (!in_extent(mouse_x, mouse_y, widget->get_extent()))
        {
            widget->set_state(widget->get_state() & (~WIDGET_STATE_FLAG_MOUSE_OVER));
            widget->receive_event(widget_event::mouse_leave());
        }
    }
    m_mouse_over_widgets.clear();

    std::queue<widget*> bfs;
    view<ui_root, camera> view(get_world());
    view.each(
        [this, &bfs](ui_root& root, camera& camera)
        {
            widget* container = root.get_container();
            bfs.push(container);
        });

    widget* hot_widget = nullptr;

    while (!bfs.empty())
    {
        widget* node = bfs.front();
        bfs.pop();

        if (!node->get_visible())
            continue;

        if (!in_extent(mouse_x, mouse_y, node->get_extent()))
            continue;

        if (!(node->get_state() & WIDGET_STATE_FLAG_MOUSE_OVER))
        {
            node->set_state(node->get_state() | WIDGET_STATE_FLAG_MOUSE_OVER);
            node->receive_event(widget_event::mouse_enter());
        }
        m_mouse_over_widgets.push_back(node);

        hot_widget = node;

        for (auto& child : node->get_children())
            bfs.push(child.get());
    }

    static const std::vector<mouse_key> keys = {MOUSE_KEY_LEFT, MOUSE_KEY_RIGHT, MOUSE_KEY_MIDDLE};

    bool key_down = false;
    for (auto key : keys)
    {
        if (mouse.key(key).press())
        {
            bubble_event(hot_widget, widget_event::mouse_press(key, mouse_x, mouse_y));
            key_down = true;
            m_mouse_press_x = mouse_x;
            m_mouse_press_y = mouse_y;

            m_drag_key = key;
            m_drag_widget = hot_widget;
        }
        else if (mouse.key(key).release())
        {
            bubble_event(hot_widget, widget_event::mouse_release(key, mouse_x, mouse_y));

            if (mouse_x == m_mouse_press_x && mouse_y == m_mouse_press_y)
            {
                m_mouse_press_x = m_mouse_press_y = -1;
                bubble_event(hot_widget, widget_event::mouse_click(key, mouse_x, mouse_y));
            }

            if (m_drag_widget != nullptr)
            {
                bubble_event(hot_widget, widget_event::drag_end(m_drag_key, mouse_x, mouse_y));
                m_drag = false;
                m_drag_widget = nullptr;
            }
        }
    }

    if (mouse.get_wheel() != 0)
        bubble_event(hot_widget, widget_event::mouse_wheel(mouse.get_wheel()));

    if (m_drag_widget != nullptr && mouse.key(m_drag_key).hold())
    {
        if (mouse_x != m_prev_mouse_x || mouse_y != m_prev_mouse_y)
        {
            if (!m_drag)
            {
                bubble_event(m_drag_widget, widget_event::drag_start(m_drag_key, mouse_x, mouse_y));
                m_drag = true;
            }
            else
            {
                bubble_event(m_drag_widget, widget_event::drag(m_drag_key, mouse_x, mouse_y));
            }
        }
    }

    m_prev_mouse_x = mouse_x;
    m_prev_mouse_y = mouse_y;

    /*if (mouse.key(MOUSE_KEY_LEFT).down())
    {
        if (m_drag_node == nullptr)
        {
            if (drag_node != nullptr)
            {
                m_drag_node = drag_node;
                if (m_drag_node->event()->on_mouse_drag_begin)
                    m_drag_node->event()->on_mouse_drag_begin(mouse_x, mouse_y);
            }
        }
        else
        {
            if (m_drag_node->event()->on_mouse_drag)
                m_drag_node->event()->on_mouse_drag(mouse_x, mouse_y);
        }
    }
    else
    {
        if (m_drag_node != nullptr)
        {
            if (m_drag_node->event()->on_mouse_drag_end)
                m_drag_node->event()->on_mouse_drag_end(mouse_x, mouse_y);
            m_drag_node = nullptr;
        }
    }

    if (focused_node != m_focused_node && key_down)
    {
        if (m_focused_node != nullptr && m_focused_node->event()->on_blur)
            m_focused_node->event()->on_blur();

        if (focused_node != nullptr && focused_node->event()->on_focus)
            focused_node->event()->on_focus();

        m_focused_node = focused_node;
    }

    if (m_focused_node != nullptr && m_focused_node->event()->on_input)
    {
        while (!m_input_chars.empty())
        {
            m_focused_node->event()->on_input(m_input_chars.front());
            m_input_chars.pop();
        }
    }
    else
    {
        while (!m_input_chars.empty())
            m_input_chars.pop();
    }*/
}

void ui_module::update_layout(widget* root, std::uint32_t width, std::uint32_t height)
{
    widget_layout* layout = root->get_layout();
    layout->calculate(width, height);

    std::stack<widget*> dfs;
    dfs.push(root);
    while (!dfs.empty())
    {
        widget* node = dfs.top();
        dfs.pop();

        widget_layout* layout = node->get_layout();

        if (layout->has_updated_flag())
        {
            node->receive_event(widget_event::layout_update());
            layout->reset_updated_flag();
        }

        std::uint32_t x = layout->get_x();
        std::uint32_t y = layout->get_y();

        for (auto& child : node->get_children())
        {
            child->get_layout()->calculate_absolute_position(x, y);
            dfs.push(child.get());
        }
    }
}

void ui_module::render(widget* root, ui_pass* pass)
{
    m_renderer->render(root, pass);
}
} // namespace violet