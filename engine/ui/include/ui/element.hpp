#pragma once

#include "graphics_interface.hpp"
#include "ui/element_layout.hpp"
#include "window/input.hpp"
#include <functional>
#include <string>

namespace ash::ui
{
template <typename T>
class element_event
{
};

template <typename R, typename... Args>
class element_event<R(Args...)>
{
public:
    class handle_impl
    {
    public:
        virtual ~handle_impl() = default;
        virtual R operator()(Args... args) = 0;
    };

    class handle_wrapper : public handle_impl
    {
    public:
        virtual R operator()(Args... args) override { return functor(args...); }
        std::function<R(Args...)> functor;
    };

    class handle
    {
    public:
        template <typename Functor>
        void set(Functor&& functor) noexcept requires std::is_invocable_v<Functor, Args...>
        {
            auto wrapper = std::make_shared<handle_wrapper>();
            wrapper->functor = functor;
            m_handle = wrapper;
        }

        void set(std::shared_ptr<handle_impl> handle) noexcept { m_handle = handle; }

        R operator()(Args... args) { return (*m_handle)(args...); }

        template <typename Functor>
        handle& operator=(Functor&& functor) noexcept requires std::is_invocable_v<Functor, Args...>
        {
            set(functor);
            return *this;
        }

        handle& operator=(std::shared_ptr<handle_impl> handle) noexcept
        {
            set(handle);
            return *this;
        }

        operator bool() const noexcept { return m_handle != nullptr; }

    private:
        std::shared_ptr<handle_impl> m_handle;
    };
};

enum element_mesh_type
{
    ELEMENT_MESH_TYPE_BLOCK,
    ELEMENT_MESH_TYPE_TEXT,
    ELEMENT_MESH_TYPE_IMAGE
};

struct element_mesh
{
    element_mesh_type type;

    const math::float2* position;
    const math::float2* uv;
    const std::uint32_t* color;
    std::size_t vertex_count;

    const std::uint32_t* indices;
    std::size_t index_count;

    bool scissor;
    graphics::resource_interface* texture;
};

class element : public element_layout
{
public:
    element(bool is_root = false);
    virtual ~element();

    const element_extent& extent() const noexcept { return m_extent; }
    void sync_extent();

    bool control_dirty() const noexcept { return m_dirty; }
    void reset_control_dirty() noexcept { m_dirty = false; }

    void add(element* child, std::size_t index = -1);
    void remove(element* child);
    void remove_from_parent();

    std::size_t link_index() const noexcept { return m_link_index; }

    element* parent() const noexcept { return m_parent; }
    const std::vector<element*>& children() const noexcept { return m_children; }

    void show();
    void hide();
    bool display() const noexcept { return m_display; }

    float depth() const noexcept { return m_depth; }
    void layer(int layer) noexcept;

    virtual const element_mesh* mesh() const noexcept { return nullptr; }

    std::string name;

public:
    using on_mouse_leave_event = element_event<void()>;
    using on_mouse_enter_event = element_event<void()>;
    using on_mouse_out_event = element_event<void()>;
    using on_mouse_over_event = element_event<void()>;
    using on_mouse_move_event = element_event<void(int, int)>;

    using on_mouse_press_event = element_event<bool(window::mouse_key, int, int)>;
    using on_mouse_release_event = element_event<bool(window::mouse_key, int, int)>;
    using on_mouse_wheel_event = element_event<bool(int)>;

    using on_mouse_drag_event = element_event<void(int, int)>;
    using on_mouse_drag_begin_event = element_event<void(int, int)>;
    using on_mouse_drag_end_event = element_event<void(int, int)>;

    using on_input_event = element_event<void(char)>;

    using on_blur_event = element_event<void()>;
    using on_focus_event = element_event<void()>;
    using on_show_event = element_event<void()>;
    using on_hide_event = element_event<void()>;

    using on_resize_event = element_event<void(int, int)>;

    on_mouse_leave_event::handle on_mouse_leave;
    on_mouse_enter_event::handle on_mouse_enter;
    on_mouse_out_event::handle on_mouse_out;
    on_mouse_over_event::handle on_mouse_over;
    on_mouse_move_event::handle on_mouse_move;
    bool mouse_over;

    on_mouse_press_event::handle on_mouse_press;
    on_mouse_release_event::handle on_mouse_release;
    on_mouse_wheel_event::handle on_mouse_wheel;

    on_mouse_drag_event::handle on_mouse_drag;
    on_mouse_drag_begin_event::handle on_mouse_drag_begin;
    on_mouse_drag_end_event::handle on_mouse_drag_end;

    on_input_event::handle on_input;

    on_blur_event::handle on_blur;
    on_focus_event::handle on_focus;

    on_show_event::handle on_show;
    on_hide_event::handle on_hide;

    on_resize_event::handle on_resize;

protected:
    virtual void on_add_child(element* child) {}
    virtual void on_remove_child(element* child) {}
    virtual void on_extent_change(float width, float height) {}

    void mark_dirty() noexcept { m_dirty = true; }

private:
    void update_depth(float depth_offset) noexcept;

    int m_layer;
    float m_depth;
    bool m_dirty;
    bool m_display;

    element_extent m_extent;

    std::size_t m_link_index;

    element* m_parent;
    std::vector<element*> m_children;
};
} // namespace ash::ui