#pragma once

#include "core/context/engine_module.hpp"
#include "window/input.hpp"
#include "window/window_extent.hpp"

namespace violet::window
{
class window_impl;
class window : public core::engine_module
{
public:
    using mouse_type = mouse;
    using keyboard_type = keyboard;

public:
    window();
    virtual ~window();

    virtual bool initialize(const dictionary& config) override;
    virtual void shutdown() override;

    void tick();

    mouse_type& get_mouse() { return m_mouse; }
    keyboard_type& get_keyboard() { return m_keyboard; }

    void* get_handle() const;
    window_extent get_extent() const;

    void set_title(std::string_view title);

private:
    std::unique_ptr<window_impl> m_impl;

    mouse_type m_mouse;
    keyboard_type m_keyboard;

    std::string m_title;
};
} // namespace violet::window