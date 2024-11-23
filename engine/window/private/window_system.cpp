#include "window/window_system.hpp"
#include "window_impl.hpp"
#include "window_impl_win32.hpp"

namespace violet
{
window_system::window_system()
    : engine_system("window"),
      m_impl(std::make_unique<window_impl_win32>()),
      m_mouse(m_impl.get())
{
}

window_system::~window_system() {}

bool window_system::initialize(const dictionary& config)
{
    if (!m_impl->initialize(config["width"], config["height"], config["title"]))
        return false;

    m_title = config["title"];

    task_graph& task_graph = get_task_graph();
    task_group& pre_update = task_graph.get_group("PreUpdate Group");

    task_graph.add_task()
        .set_name("Update Window")
        .set_group(pre_update)
        .set_options(TASK_OPTION_MAIN_THREAD)
        .set_execute(
            [this]()
            {
                tick();
            });

    return true;
}

void window_system::shutdown()
{
    m_impl->shutdown();
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

void window_system::tick()
{
    m_impl->reset();
    m_impl->tick();

    auto& executor = get_executor();

    m_mouse.tick();
    m_keyboard.tick();
    for (auto& message : m_impl->get_messages())
    {
        switch (message.type)
        {
        case window_message::message_type::MOUSE_MOVE: {
            m_mouse.m_x = message.mouse_move.x;
            m_mouse.m_y = message.mouse_move.y;
            break;
        }
        case window_message::message_type::MOUSE_KEY: {
            if (message.mouse_key.down)
                m_mouse.key_down(message.mouse_key.key);
            else
                m_mouse.key_up(message.mouse_key.key);
            break;
        }
        case window_message::message_type::MOUSE_WHELL: {
            m_mouse.m_wheel = message.mouse_wheel;
            break;
        }
        case window_message::message_type::KEYBOARD_KEY: {
            if (message.keyboard_key.down)
                m_keyboard.key_down(message.keyboard_key.key);
            else
                m_keyboard.key_up(message.keyboard_key.key);
            break;
        }
        case window_message::message_type::KEYBOARD_CHAR: {
            break;
        }
        case window_message::message_type::WINDOW_MOVE: {
            break;
        }
        case window_message::message_type::WINDOW_RESIZE: {
            get_executor().execute_sync(m_on_resize);
            break;
        }
        case window_message::message_type::WINDOW_DESTROY: {
            get_executor().execute_sync(m_on_destroy);
            break;
        }
        default:
            break;
        }
    }
}
} // namespace violet