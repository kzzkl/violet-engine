#include "window/window.hpp"
#include "core/context/engine.hpp"
#include "window/window_event.hpp"
#include "window_impl.hpp"
#include "window_impl_win32.hpp"

namespace violet::window
{
window::window()
    : core::engine_module("window"),
      m_impl(std::make_unique<window_impl_win32>()),
      m_mouse(m_impl.get())
{
}

window::~window()
{
}

bool window::initialize(const dictionary& config)
{
    if (!m_impl->initialize(config["width"], config["height"], config["title"]))
        return false;

    m_title = config["title"];

    auto& event = core::engine::get_event();
    event.register_event<event_mouse_move>();
    event.register_event<event_mouse_key>();
    event.register_event<event_keyboard_key>();
    event.register_event<event_keyboard_char>();
    event.register_event<event_window_resize>();
    event.register_event<event_window_destroy>();

    auto& task = core::engine::get_task_manager();
    auto window_tick_task = task.schedule(
        "window_tick",
        [this]() { tick(); },
        core::task_type::MAIN_THREAD);
    window_tick_task->add_dependency(*task.find(core::TASK_ROOT));

    auto logic_start_task = task.find(core::TASK_GAME_LOGIC_START);
    logic_start_task->add_dependency(*window_tick_task);

    return true;
}

void window::shutdown()
{
    auto& event = core::engine::get_event();
    event.unregister_event<event_mouse_move>();
    event.unregister_event<event_mouse_key>();
    event.unregister_event<event_keyboard_key>();
    event.unregister_event<event_keyboard_char>();
    event.unregister_event<event_window_resize>();
    event.unregister_event<event_window_destroy>();

    m_impl->shutdown();
}

void window::tick()
{
    auto& event = core::engine::get_event();

    m_mouse.tick();
    m_keyboard.tick();

    m_impl->reset();
    m_impl->tick();

    for (auto& message : m_impl->get_messages())
    {
        switch (message.type)
        {
        case window_message::message_type::MOUSE_MOVE:
            m_mouse.m_x = message.mouse_move.x;
            m_mouse.m_y = message.mouse_move.y;
            event.publish<event_mouse_move>(
                m_mouse.get_mode(),
                message.mouse_move.x,
                message.mouse_move.y);
            break;
        case window_message::message_type::MOUSE_KEY:
            if (message.mouse_key.down)
                m_mouse.key_down(message.mouse_key.key);
            else
                m_mouse.key_up(message.mouse_key.key);
            event.publish<event_mouse_key>(
                message.mouse_key.key,
                m_mouse.key(message.mouse_key.key));
            break;
        case window_message::message_type::MOUSE_WHELL:
            m_mouse.m_whell = message.mouse_whell;
            break;
        case window_message::message_type::KEYBOARD_KEY:
            if (message.keyboard_key.down)
                m_keyboard.key_down(message.keyboard_key.key);
            else
                m_keyboard.key_up(message.keyboard_key.key);
            event.publish<event_keyboard_key>(
                message.keyboard_key.key,
                m_keyboard.key(message.keyboard_key.key));
            break;
        case window_message::message_type::KEYBOARD_CHAR:
            event.publish<event_keyboard_char>(message.keyboard_char);
            break;
        case window_message::message_type::WINDOW_MOVE:
            break;
        case window_message::message_type::WINDOW_RESIZE:
            event.publish<event_window_resize>(
                message.window_resize.width,
                message.window_resize.height);
            break;
        case window_message::message_type::WINDOW_DESTROY:
            event.publish<event_window_destroy>();
            break;
        default:
            break;
        }
    }
}

void* window::get_handle() const
{
    return m_impl->get_handle();
}

window_extent window::get_extent() const
{
    return m_impl->get_extent();
}

void window::set_title(std::string_view title)
{
    m_title = title;
    m_impl->set_title(title);
}
} // namespace violet::window