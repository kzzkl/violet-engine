#pragma once

#include "ui/control_mesh.hpp"
#include "ui/event.hpp"
#include "ui/layout/layout.hpp"
#include <memory>
#include <string>

namespace violet
{
class control
{
public:
    control(bool is_root = false);
    virtual ~control();

    void sync_extent();

    bool control_dirty() const noexcept { return m_dirty; }
    void reset_control_dirty() noexcept { m_dirty = false; }

    void add(control* child, std::size_t index = -1);
    void remove(control* child);
    void remove_from_parent();

    std::size_t link_index() const noexcept { return m_link_index; }

    control* parent() const noexcept { return m_parent; }
    const std::vector<control*>& children() const noexcept { return m_children; }

    void show();
    void hide();
    bool display() const noexcept { return m_display; }

    float depth() const noexcept { return m_depth; }
    void layer(int layer) noexcept;

    virtual const control_mesh* mesh() const noexcept { return nullptr; }

    std::string name;

    layout_node* layout() const noexcept { return m_layout.get(); }
    event_node* event() const noexcept { return m_event.get(); }

protected:
    virtual void on_add_child(control* child) {}
    virtual void on_remove_child(control* child) {}
    virtual void on_extent_change(float width, float height) {}

    void mark_dirty() noexcept { m_dirty = true; }

private:
    void update_depth(float depth_offset) noexcept;

    int m_layer;
    float m_depth;
    bool m_dirty;
    bool m_display;

    std::size_t m_link_index;

    control* m_parent;
    std::vector<control*> m_children;

    std::unique_ptr<layout_node> m_layout;
    std::unique_ptr<event_node> m_event;
};
} // namespace violet