#include "ui.hpp"
#include "graphics.hpp"
#include "relation.hpp"
#include "ui_impl_imgui.hpp"
#include "window.hpp"

namespace ash::ui
{
ui::ui() : core::system_base("ui")
{
}

bool ui::initialize(const dictionary& config)
{
    auto& graphics = system<graphics::graphics>();
    auto& world = system<ecs::world>();

    ui_impl_desc desc = {};
    desc.window_handle = system<window::window>().handle();
    desc.graphics = &graphics;
    m_impl = std::make_unique<ui_impl_imgui>(desc);

    m_pipeline = graphics.make_render_pipeline<graphics::render_pipeline>("ui");

    m_ui_entity = world.create();
    world.add<graphics::visual>(m_ui_entity);

    return true;
}

void ui::begin_frame()
{
    auto& window = system<window::window>();

    window::window_rect rect = window.rect();

    static auto old_time = system<core::timer>().now();
    auto new_time = system<core::timer>().now();
    float delta = (new_time - old_time).count() * 0.000000001f;
    old_time = new_time;

    m_impl->begin_frame(rect.width, rect.height, delta);
}

void ui::end_frame()
{
    m_impl->end_frame();
    render();
}

void ui::render()
{
    auto& world = system<ecs::world>();

    auto& visual = world.component<graphics::visual>(m_ui_entity);
    visual.submesh.clear();

    for (auto& submesh : m_impl->render_data())
    {
        visual.submesh.push_back(submesh);
        visual.submesh.back().pipeline = m_pipeline.get();
    }
}
} // namespace ash::ui