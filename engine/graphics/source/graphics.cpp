#include "graphics.hpp"
#include "log.hpp"

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

    diagnotor* d = m_plugin.get_context()->get_diagnotor();

    adapter_info info[4] = {};
    int num_adapter = d->get_adapter_info(info, 4);
    for (int i = 0; i < num_adapter; ++i)
    {
        log::debug("graphics adapter: {}", info[i].description);
    }

    return true;
}
} // namespace ash::graphics