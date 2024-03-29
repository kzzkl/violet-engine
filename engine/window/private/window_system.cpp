#include "window/window_system.hpp"
#include "window_impl.hpp"
#include "window_impl_win32.hpp"

namespace violet
{
window_system::window_system()
    : engine_system("window"),
      m_impl(std::make_unique<window_impl_win32>()),
      m_mouse(m_impl.get()),
      m_on_tick(nullptr)
{
}

window_system::~window_system()
{
}

bool window_system::initialize(const dictionary& config)
{
    if (!m_impl->initialize(config["width"], config["height"], config["title"]))
        return false;

    m_title = config["title"];

    m_on_tick = &on_frame_begin().then(
        [this]()
        {
            tick();
        },
        TASK_OPTION_MAIN_THREAD);

    return true;
}

void window_system::shutdown()
{
    m_impl->shutdown();
}

void window_system::tick()
{
    auto& executor = get_task_executor();

    m_mouse.tick();
    m_keyboard.tick();

    m_impl->reset();
    m_impl->tick();

    for (auto& message : m_impl->get_messages())
    {
        switch (message.type)
        {
        case window_message::message_type::MOUSE_MOVE: {
            m_mouse.m_x = message.mouse_move.x;
            m_mouse.m_y = message.mouse_move.y;

            executor.execute_sync(
                m_on_mouse_move,
                m_mouse.get_mode(),
                message.mouse_move.x,
                message.mouse_move.y);
            break;
        }
        case window_message::message_type::MOUSE_KEY: {
            if (message.mouse_key.down)
                m_mouse.key_down(message.mouse_key.key);
            else
                m_mouse.key_up(message.mouse_key.key);

            executor.execute_sync(
                m_on_mouse_key,
                message.mouse_key.key,
                m_mouse.key(message.mouse_key.key));
            break;
        }
        case window_message::message_type::MOUSE_WHELL: {
            m_mouse.m_whell = message.mouse_whell;
            break;
        }
        case window_message::message_type::KEYBOARD_KEY: {
            if (message.keyboard_key.down)
                m_keyboard.key_down(message.keyboard_key.key);
            else
                m_keyboard.key_up(message.keyboard_key.key);

            executor.execute_sync(
                m_on_keyboard_key,
                message.keyboard_key.key,
                m_keyboard.key(message.keyboard_key.key));
            break;
        }
        case window_message::message_type::KEYBOARD_CHAR: {
            executor.execute_sync(m_on_keyboard_char, message.keyboard_char);
            break;
        }
        case window_message::message_type::WINDOW_MOVE: {
            break;
        }
        case window_message::message_type::WINDOW_RESIZE: {
            executor.execute_sync(
                m_on_resize,
                message.window_resize.width,
                message.window_resize.height);
            break;
        }
        case window_message::message_type::WINDOW_DESTROY: {
            executor.execute_sync(m_on_destroy);
            break;
        }
        default:
            break;
        }
    }
}

void* window_system::get_handle() const
{
    return m_impl->get_handle();
}

rect<std::uint32_t> window_system::get_extent() const
{
    return m_impl->get_extent();
}

void window_system::set_title(std::string_view title)
{
    m_title = title;
    m_impl->set_title(title);
}
} // namespace violet