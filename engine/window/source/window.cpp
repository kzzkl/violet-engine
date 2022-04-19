#include "window.hpp"
#include "context.hpp"
#include "window_impl_win32.hpp"

using namespace ash::core;

namespace ash::window
{
window::window() : system_base("window")
{
    m_impl = std::make_unique<window_impl_win32>();
}

bool window::initialize(const dictionary& config)
{
    if (!m_impl->initialize(config["width"], config["height"], config["title"]))
        return false;

    system<task::task_manager>().schedule(
        TASK_WINDOW_TICK,
        [this]() { m_impl->tick(); },
        task::task_type::MAIN_THREAD);

    return true;
}
} // namespace ash::window