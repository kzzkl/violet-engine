#pragma once

#include "math/math.hpp"
#include "ui/element_layout.hpp"
#include "ui/element_mesh.hpp"
#include "ui/renderer.hpp"
#include "window/input.hpp"

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

    void layer(std::uint32_t layer) noexcept { m_layer = layer; }
    std::uint32_t layer() const noexcept { return m_layer; }

    const element_mesh& mesh() const noexcept { return m_mesh; }

    bool control_dirty() const noexcept { return m_dirty; }
    void reset_control_dirty() noexcept { m_dirty = false; }

    void link(element* parent);
    element* parent() { return m_parent; }
    const std::vector<element*>& children() { return m_children; }

    bool show;

public:
    virtual void on_mouse_enter() {}
    virtual void on_mouse_exit() {}
    virtual void on_mouse_click(window::mouse_key key) {}
    virtual void on_hover() {}

    virtual void on_blur() {}
    virtual void on_focus() {}

    virtual void on_show() {}
    virtual void on_hide() {}

    virtual void on_add_child(element* child);
    virtual void on_remove_child(element* child);

    virtual void on_extent_change(const element_extent& extent) {}

protected:
    void mark_dirty() noexcept { m_dirty = true; }
    float depth() const noexcept { return 1.0f - static_cast<float>(m_layer) * 0.01f; }

    element_mesh m_mesh;

private:
    std::uint32_t m_layer;
    bool m_dirty;

    element* m_parent;
    std::vector<element*> m_children;
};
} // namespace ash::ui