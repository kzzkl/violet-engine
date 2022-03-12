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
    dictionary merge = dictionary::object();
    for (auto& c : config)
        merge.insert(c.cbegin(), c.cend());

    if (!m_impl->initialize(merge["width"], merge["height"], merge["title"]))
        return false;

    get_submodule<task::task_manager>().schedule_before("window tick", [this]() {
        m_impl->tick();
    });

    return true;
}
} // namespace ash::window