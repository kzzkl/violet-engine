#include "window.hpp"
#include "context.hpp"
#include "window_impl_win32.hpp"

using namespace ash::core;

namespace ash::window
{
window::window() : submodule("window")
{
    m_impl = std::make_unique<window_impl_win32>();
}

bool window::initialize(const dictionary& config)
{
    if (!m_impl->initialize(config["width"], config["height"], config["title"]))
        return false;

    auto task_handle = get_submodule<task::task_manager>().schedule(
        "window tick",
        [this]() { m_impl->tick(); },
        task::task_type::MAIN_THREAD);

    task_handle->add_dependency(*get_submodule<task::task_manager>().find("root"));

    return true;
}
} // namespace ash::window