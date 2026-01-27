#pragma once

#include "core/engine.hpp"
#include "math/rect.hpp"
#include "task/task.hpp"
#include "window/input.hpp"

namespace violet
{
class window_impl;
class window_system : public system
{
public:
    using mouse_type = mouse;
    using keyboard_type = keyboard;

    window_system();
    virtual ~window_system();

    bool initialize(const dictionary& config) override;
    void shutdown() override;

    mouse_type& get_mouse()
    {
        return m_mouse;
    }

    keyboard_type& get_keyboard()
    {
        return m_keyboard;
    }

    void* get_handle() const;

    rect<std::uint32_t> get_window_size() const;
    rect<std::uint32_t> get_screen_size() const;

    void set_title(std::string_view title);

    task_graph& on_resize()
    {
        return m_on_resize;
    }

    task_graph& on_destroy()
    {
        return m_on_destroy;
    }

private:
    void tick();

    std::unique_ptr<window_impl> m_impl;

    mouse_type m_mouse;
    keyboard_type m_keyboard;

    std::string m_title;

    task_graph m_on_resize;
    task_graph m_on_destroy;
};
} // namespace violet