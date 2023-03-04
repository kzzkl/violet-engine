#pragma once

#include "window/input.hpp"
#include <functional>

namespace violet::ui
{
template <typename T>
class control_event
{
};

template <typename R, typename... Args>
class control_event<R(Args...)>
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
        void set(Functor&& functor) noexcept
            requires std::is_invocable_v<Functor, Args...>
        {
            auto wrapper = std::make_shared<handle_wrapper>();
            wrapper->functor = functor;
            m_handle = wrapper;
        }

        void set(std::shared_ptr<handle_impl> handle) noexcept { m_handle = handle; }

        R operator()(Args... args) { return (*m_handle)(args...); }

        template <typename Functor>
        handle& operator=(Functor&& functor) noexcept
            requires std::is_invocable_v<Functor, Args...>
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

enum event_type
{
    EVENT_TYPE_MOUSE_MOVE,
    EVENT_TYPE_MOUSE_LEAVE,
    EVENT_TYPE_MOUSE_ENTER,
    EVENT_TYPE_MOUSE_OUT,
    EVENT_TYPE_MOUSE_OVER,

    EVENT_TYPE_MOUSE_PRESS,
    EVENT_TYPE_MOUSE_RELEASE,
    EVENT_TYPE_MOUSE_WHEEL,

    EVENT_TYPE_MOUSE_DRAG,
    EVENT_TYPE_MOUSE_DRAG_BEGIN,
    EVENT_TYPE_MOUSE_DRAG_END,

    EVENT_TYPE_BLUR,
    EVENT_TYPE_FOCUS,
    EVENT_TYPE_SHOW,
    EVENT_TYPE_HIDE,

    EVENT_TYPE_INPUT,
    EVENT_TYPE_RESIZE
};

struct event
{
    event_type type;

    union {
        struct
        {
            int x;
            int y;
        } mouse_move;

        struct
        {
            window::mouse_key key;
            int x;
            int y;
        } key;

        char input;

        struct
        {
            std::uint32_t width;
            std::uint32_t height;
        } resize;
    };
};

class event_listener
{
public:
    virtual ~event_listener() = default;

    virtual bool receive();
};

class event_node
{
public:
    event_node();

    using on_mouse_leave_event = control_event<void()>;
    using on_mouse_enter_event = control_event<void()>;
    using on_mouse_out_event = control_event<void()>;
    using on_mouse_over_event = control_event<void()>;
    using on_mouse_move_event = control_event<void(int, int)>;

    using on_mouse_press_event = control_event<bool(window::mouse_key, int, int)>;
    using on_mouse_release_event = control_event<bool(window::mouse_key, int, int)>;
    using on_mouse_wheel_event = control_event<bool(int)>;

    using on_mouse_drag_event = control_event<void(int, int)>;
    using on_mouse_drag_begin_event = control_event<void(int, int)>;
    using on_mouse_drag_end_event = control_event<void(int, int)>;

    using on_input_event = control_event<void(char)>;

    using on_blur_event = control_event<void()>;
    using on_focus_event = control_event<void()>;
    using on_show_event = control_event<void()>;
    using on_hide_event = control_event<void()>;

    using on_resize_event = control_event<void(int, int)>;

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
};
} // namespace violet::ui