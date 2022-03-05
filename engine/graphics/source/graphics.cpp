#include "graphics.hpp"
#include "context.hpp"
#include "log.hpp"
#include "window.hpp"

using namespace ash::graphics::external;
using namespace ash::common;

namespace ash::graphics
{
graphics::graphics() : submodule("graphics")
{
}

bool graphics::initialize(const ash::common::dictionary& config)
{
    if (!m_plugin.load("ash-graphics-d3d12.dll"))
        return false;

    graphics_context* context = m_plugin.get_context();

    if (!context->initialize(get_config(config)))
    {
        log::error("Graphics context initialization failed.");
        return false;
    }

    diagnotor* d = context->get_diagnotor();

    adapter_info info[4] = {};
    int num_adapter = d->get_adapter_info(info, 4);
    for (int i = 0; i < num_adapter; ++i)
    {
        log::debug("graphics adapter: {}", info[i].description);
    }

    m_renderer = context->get_renderer();

    initialize_task();

    return true;
}

void graphics::initialize_task()
{
    auto& task = get_context().get_task();

    auto root_task = task.find("root");
    auto render_task = task.schedule("render", [this]() {
        m_renderer->begin_frame();
        m_renderer->end_frame();
    });
    render_task->add_dependency(*root_task);
}

graphics::config graphics::get_config(const ash::common::dictionary& config)
{
    graphics_context_config result = {};

    auto& window = get_context().get_submodule<ash::window::window>();

    result.handle = window.get_handle();

    window::window_rect rect = window.get_rect();
    result.width = rect.width;
    result.height = rect.height;

    return result;
}
} // namespace ash::graphics