#include "graphics/graphics.hpp"
#include "components/mesh.hpp"
#include "components/transform.hpp"
#include "core/context/engine.hpp"
#include "rhi/rhi_plugin.hpp"
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
    rect<std::uint32_t> extent = engine_window.get_extent();

    rhi_desc rhi_desc = {};
    rhi_desc.width = extent.width;
    rhi_desc.height = extent.height;
    rhi_desc.window_handle = engine_window.get_handle();
    rhi_desc.render_concurrency = config["render_concurrency"];
    rhi_desc.frame_resource = config["frame_resource"];

    m_plugin = std::make_unique<rhi_plugin>();
    m_plugin->load(config["plugin"]);
    m_plugin->get_rhi()->initialize(rhi_desc);

    return true;
}

void graphics::render()
{
    view<mesh, transform> v(engine::get_world());

    v.each([](mesh& mesh, transform& transform) {
        if (transform.get_update_count() != 0)
            mesh.node_parameter->set_world_matrix(transform.get_world_matrix());
    });

    get_rhi()->present();
}

rhi_interface* graphics::get_rhi() const
{
    return m_plugin->get_rhi();
}
} // namespace violet