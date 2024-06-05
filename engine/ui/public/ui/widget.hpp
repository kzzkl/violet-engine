#pragma once

#include "ui/event/event.hpp"
#include "ui/layout/layout.hpp"
#include "ui/rendering/ui_painter.hpp"
#include "window/input.hpp"
#include <memory>
#include <vector>

namespace violet
{
using widget_extent = rect<float>;

enum widget_state_flag : std::uint32_t
{
    WIDGET_STATE_FLAG_MOUSE_OVER = 1 << 0
};
using widget_state_flags = std::uint32_t;

class widget
{
public:
    widget();
    virtual ~widget();

    template <typename T, typename... Args>
    T* add(Args&&... args)
    {
        auto child = std::make_unique<T>(std::forward<Args>(args)...);
        T* result = child.get();

        m_layout->add_child(child->get_layout(), m_children.size());
        child->m_layer = m_layer + 1;
        m_children.push_back(std::move(child));

        return result;
    }

    template <typename T>
    std::unique_ptr<T> remove(T* child)
    {
        auto iter = std::find_if(
            m_children.begin(),
            m_children.end(),
            [child](auto& c)
            {
                return c.get() == child;
            });
        if (iter != m_children.end())
        {
            m_layout->remove_child(child->get_layout());

            std::unique_ptr<T> result(static_cast<T*>((*iter).release()));
            m_children.erase(iter);
            return result;
        }
        else
        {
            return nullptr;
        }
    }

    widget_state_flags get_state() const noexcept { return m_state; }
    void set_state(widget_state_flags state) noexcept { m_state = state; }

    void set_visible(bool visible) noexcept { m_visible = visible; }
    bool get_visible() const noexcept { return m_visible; }

    widget_extent get_extent() const;

    widget_layout* get_layout() const noexcept { return m_layout.get(); }

    widget* get_parent() const noexcept { return m_parent; }
    const std::vector<std::unique_ptr<widget>>& get_children() const noexcept { return m_children; }

    bool receive_event(const widget_event& event);

private:
    virtual void on_paint(ui_painter* painter) {}
    virtual void on_paint_end(ui_painter* painter) {}
    virtual void on_layout_update() {}

    virtual bool on_mouse_move(int x, int y) { return true; }
    virtual bool on_mouse_enter() { return true; }
    virtual bool on_mouse_leave() { return true; }
    virtual bool on_mouse_press(mouse_key key, int x, int y) { return true; }
    virtual bool on_mouse_release(mouse_key key, int x, int y) { return true; }
    virtual bool on_mouse_wheel(int wheel) { return true; }
    virtual bool on_mouse_click(mouse_key key, int x, int y) { return true; }

    virtual bool on_drag_start(mouse_key key, int x, int y) { return true; }
    virtual bool on_drag(mouse_key key, int x, int y) { return true; }
    virtual bool on_drag_end(mouse_key key, int x, int y) { return true; }

    widget_state_flags m_state;

    bool m_visible;

    std::size_t m_layer;

    widget* m_parent;
    std::vector<std::unique_ptr<widget>> m_children;

    std::unique_ptr<widget_layout> m_layout;
};
} // namespace violet