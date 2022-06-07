#pragma once

#include "math/math.hpp"
#include "ui/element_layout.hpp"
#include "ui/element_mesh.hpp"
#include "ui/renderer.hpp"
#include "window/input.hpp"
#include <functional>

namespace ash::ui
{
class element : public element_layout
{
public:
    element(bool is_root = false);
    virtual ~element();

    virtual void render(renderer& renderer);

    const element_extent& extent() const noexcept { return m_extent; }
    void sync_extent();

    const element_mesh& mesh() const noexcept { return m_mesh; }

    bool control_dirty() const noexcept { return m_dirty; }
    void reset_control_dirty() noexcept { m_dirty = false; }

    void link(element* parent);
    void link(element* parent, std::size_t index);
    void unlink();

    std::size_t link_index() const noexcept { return m_link_index; }

    element* parent() const noexcept { return m_parent; }
    const std::vector<element*>& children() const noexcept { return m_children; }

    void show();
    void hide();
    bool display() const noexcept { return m_display; }

    float depth() const noexcept { return m_depth; }
    void layer(int layer) noexcept;

    virtual bool dockable() const noexcept { return false; }

    std::string name;

public:
    bool mouse_over;

    std::function<void()> on_mouse_leave;
    std::function<void()> on_mouse_enter;
    std::function<void()> on_mouse_out;
    std::function<void()> on_mouse_over;
    std::function<bool(window::mouse_key key, int x, int y)> on_mouse_press;
    std::function<bool(window::mouse_key key, int x, int y)> on_mouse_release;
    std::function<bool(int whell)> on_mouse_wheel;

    std::function<void(int x, int y)> on_mouse_drag_begin;
    std::function<void(int x, int y)> on_mouse_drag;
    std::function<void(int x, int y)> on_mouse_drag_end;

    std::function<void()> on_blur;
    std::function<void()> on_focus;

    std::function<void()> on_show;
    std::function<void()> on_hide;

protected:
    virtual void on_extent_change() {}

    virtual void on_add_child(element* child);
    virtual void on_remove_child(element* child);

    void mark_dirty() noexcept { m_dirty = true; }

    element_mesh m_mesh;

private:
    void update_depth(float parent_depth) noexcept;

    int m_layer;
    float m_depth;
    bool m_dirty;
    bool m_display;

    element_extent m_extent;

    std::size_t m_link_index;

    element* m_parent;
    std::vector<element*> m_children;
};

class dock_element : public element
{
public:
    dock_element();

    void dock(dock_element* target, layout_edge edge);
    void undock();

    virtual bool dockable() const noexcept { return true; }

private:
    void move_up();
    void move_down();

    bool is_root() const noexcept { return m_dock_node == nullptr; }

    std::shared_ptr<dock_element> m_dock_node;
};
} // namespace ash::ui