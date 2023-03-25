#include "graphics/graphics.hpp"
#include "core/context/engine.hpp"
#include "window/window.hpp"

namespace violet
{
graphics::graphics() : engine_module("graphics")
{
}

graphics::~graphics()
{
}

bool graphics::initialize(const dictionary& config)
{
    auto& engine_window = engine::get_module<window>();

    rhi_desc desc = {};

    rect<std::uint32_t> extent = engine_window.get_extent();
    desc.width = extent.width;
    desc.height = extent.height;
    desc.window_handle = engine_window.get_handle();

    desc.render_concurrency = config["render_concurrency"];
    desc.frame_resource = config["frame_resource"];

    m_rhi = std::make_unique<rhi>(config["plugin"], desc);

    return true;
}
} // namespace violet