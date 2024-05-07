#pragma once

#include "core/engine_system.hpp"
#include "math/rect.hpp"
#include "task/task.hpp"
#include "window/input.hpp"

namespace violet
{
class window_impl;
class window_system : public engine_system
{
public:
    using mouse_type = mouse;
    using keyboard_type = keyboard;

public:
    window_system();
    virtual ~window_system();

    virtual bool initialize(const dictionary& config) override;
    virtual void shutdown() override;

    mouse_type& get_mouse() { return m_mouse; }
    keyboard_type& get_keyboard() { return m_keyboard; }

    void* get_handle() const;
    rect<std::uint32_t> get_extent() const;

    void set_title(std::string_view title);

    task<std::uint32_t, std::uint32_t>& on_resize() { return m_on_resize.get_root(); }
    task<>& on_destroy() { return m_on_destroy.get_root(); }

private:
    void tick();

    std::unique_ptr<window_impl> m_impl;

    mouse_type m_mouse;
    keyboard_type m_keyboard;

    std::string m_title;

    task_graph<std::uint32_t, std::uint32_t> m_on_resize;
    task_graph<> m_on_destroy;
};
} // namespace violet