#pragma once

#include "context.hpp"
#include "window_impl.hpp"

namespace ash::window
{
class window : public ash::core::system_base
{
public:
    static constexpr const char* TASK_WINDOW_TICK = "window tick";

    using mouse_type = mouse;
    using keyboard_type = keyboard;

public:
    window();

    virtual bool initialize(const dictionary& config) override;

    mouse_type& mouse() { return m_mouse; }
    keyboard_type& keyboard() { return m_keyboard; }

    void* handle() const { return m_impl->handle(); }
    window_rect rect() const { return m_impl->rect(); }

    void title(std::string_view title) { m_impl->title(title); }

private:
    void process_message();

    std::unique_ptr<window_impl> m_impl;

    mouse_type m_mouse;
    keyboard_type m_keyboard;
};
} // namespace ash::window