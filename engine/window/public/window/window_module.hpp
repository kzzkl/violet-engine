#pragma once

#include "core/context/engine_module.hpp"
#include "math/rect.hpp"
#include "window/input.hpp"
#include "window/window_task.hpp"

namespace violet
{
class window_impl;
class window_module : public engine_module
{
public:
    using mouse_type = mouse;
    using keyboard_type = keyboard;

public:
    window_module();
    virtual ~window_module();

    virtual bool initialize(const dictionary& config) override;
    virtual void shutdown() override;

    void tick();

    mouse_type& get_mouse() { return m_mouse; }
    keyboard_type& get_keyboard() { return m_keyboard; }

    void* get_handle() const;
    rect<std::uint32_t> get_extent() const;

    void set_title(std::string_view title);
    
    window_task_graph& get_task_graph() { return m_task_graph; }

private:
    std::unique_ptr<window_impl> m_impl;

    mouse_type m_mouse;
    keyboard_type m_keyboard;

    std::string m_title;

    window_task_graph m_task_graph;
};
} // namespace violet