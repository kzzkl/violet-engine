#include "graphics_config.hpp"
#include "log.hpp"

namespace ash::graphics
{
graphics_config::graphics_config()
{
}

void graphics_config::load(const dictionary& config)
{
    m_render_concurrency = config["render_concurrency"];
    m_frame_resource = config["frame_resource"];
    m_multiple_sampling = config["multiple_sampling"];

    m_plugin = config["plugin"];
}
} // namespace ash::graphics