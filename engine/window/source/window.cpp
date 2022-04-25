#include "window.hpp"
#include "context.hpp"
#include "window_event.hpp"
#include "window_impl_win32.hpp"

using namespace ash::core;

namespace ash::window
{
window::window()
    : system_base("window"),
      m_impl(std::make_unique<window_impl_win32>()),
      m_mouse(m_impl.get())
{
}

bool window::initialize(const dictionary& config)
{
    if (!m_impl->initialize(config["width"], config["height"], config["title"]))
        return false;

    auto& event = system<core::event>();
    event.register_event<event_mouse_move>();
    event.register_event<event_mouse_key>();
    event.register_event<event_keyboard_key>();
    event.register_event<event_keyboard_char>();

    return true;
}

void window::tick()
{
    auto& event = system<core::event>();

    m_mouse.tick();
    m_keyboard.tick();

    m_impl->reset();
    m_impl->tick();

    for (auto& message : m_impl->messages())
    {
        switch (message.type)
        {
        case window_message::message_type::MOUSE_MOVE:
            m_mouse.m_x = message.mouse_move.x;
            m_mouse.m_y = message.mouse_move.y;
            event.publish<event_mouse_move>(
                m_mouse.mode(),
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
            break;
        default:
            break;
        }
    }
}
} // namespace ash::window