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

bool window::initialize(const ash::common::dictionary& config)
{
    std::string title = "ash app";
    uint32_t width = 800;
    uint32_t height = 600;

    auto iter = config.find("window");
    if (iter != config.cend())
    {
        if (iter->find("title") != iter->cend())
            title = (*iter)["title"];

        if (iter->find("width") != iter->cend())
            width = (*iter)["width"];

        if (iter->find("height") != iter->cend())
            height = (*iter)["height"];
    }

    if (!m_impl->initialize(width, height, title))
        return false;

    get_context().get_task().schedule_before("window tick", [this]() { m_impl->tick(); });

    return true;
}
} // namespace ash::window