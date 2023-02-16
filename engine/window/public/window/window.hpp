#pragma once

#include "core/context.hpp"
#include "window/input.hpp"
#include "window/window_extent.hpp"

namespace violet::window
{
class window_impl;
class window : public core::system_base
{
public:
    using mouse_type = mouse;
    using keyboard_type = keyboard;

public:
    window();

    virtual bool initialize(const dictionary& config) override;
    virtual void shutdown() override;

    void tick();

    mouse_type& mouse() { return m_mouse; }
    keyboard_type& keyboard() { return m_keyboard; }

    void* handle() const;
    window_extent extent() const;

    void title(std::string_view title);

private:
    std::unique_ptr<window_impl> m_impl;

    mouse_type m_mouse;
    keyboard_type m_keyboard;

    std::string m_title;

#if defined(VIOLET_WINDOW_SHOW_FPS)
    float m_average_duration;
    int m_fps;
#endif
};
} // namespace violet::window