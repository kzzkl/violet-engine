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
    virtual ~element() = default;

    virtual void tick() {}
    virtual void render(renderer& renderer);

    element_extent extent() const { return layout_extent(); }

    const element_mesh& mesh() const noexcept { return m_mesh; }

    bool control_dirty() const noexcept { return m_dirty; }
    void reset_control_dirty() noexcept { m_dirty = false; }

    void link(element* parent);
    element* parent() { return m_parent; }
    const std::vector<element*>& children() { return m_children; }

    void show();
    void hide();
    bool display() const noexcept { return m_display; }

public:
    std::function<void()> on_mouse_enter;
    std::function<void()> on_mouse_exit;
    std::function<bool(window::mouse_key)> on_mouse_click;
    std::function<void()> on_hover;

    std::function<void()> on_blur;
    std::function<void()> on_focus;

    std::function<void()> on_show;
    std::function<void()> on_hide;

    virtual void on_extent_change(const element_extent& extent) {}

protected:
    virtual void on_add_child(element* child);
    virtual void on_remove_child(element* child);

    void mark_dirty() noexcept { m_dirty = true; }
    float depth() const noexcept { return m_depth; }

    element_mesh m_mesh;

private:
    void update_depth(float parent_depth);

    float m_depth;
    bool m_dirty;
    bool m_display;

    element* m_parent;
    std::vector<element*> m_children;
};
} // namespace ash::ui