#pragma once

#include "core/engine_system.hpp"
#include "core/task/task.hpp"
#include "math/rect.hpp"
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

    void tick();

    mouse_type& get_mouse() { return m_mouse; }
    keyboard_type& get_keyboard() { return m_keyboard; }

    void* get_handle() const;
    rect<std::uint32_t> get_extent() const;

    void set_title(std::string_view title);

    task<>& on_tick() { return *m_on_tick; }

    task<mouse_mode, int, int>& on_mouse_move() { return m_on_mouse_move.get_root(); }
    task<mouse_key, key_state>& on_mouse_key() { return m_on_mouse_key.get_root(); }
    task<keyboard_key, key_state>& on_keyboard_key() { return m_on_keyboard_key.get_root(); }
    task<char>& on_keyboard_char() { return m_on_keyboard_char.get_root(); }
    task<std::uint32_t, std::uint32_t>& on_resize() { return m_on_resize.get_root(); }
    task<>& on_destroy() { return m_on_destroy.get_root(); }

private:
    std::unique_ptr<window_impl> m_impl;

    mouse_type m_mouse;
    keyboard_type m_keyboard;

    std::string m_title;

    task_graph<mouse_mode, int, int> m_on_mouse_move;
    task_graph<mouse_key, key_state> m_on_mouse_key;
    task_graph<keyboard_key, key_state> m_on_keyboard_key;
    task_graph<char> m_on_keyboard_char;
    task_graph<std::uint32_t, std::uint32_t> m_on_resize;
    task_graph<> m_on_destroy;

    task<>* m_on_tick;
};
} // namespace violet