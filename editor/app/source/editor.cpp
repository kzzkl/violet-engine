#include "editor/editor.hpp"
#include "core/relation.hpp"
#include "editor/editor_task.hpp"
#include "graphics/camera.hpp"
#include "graphics/graphics.hpp"
#include "graphics/rhi.hpp"
#include "scene/scene.hpp"
#include "task/task_manager.hpp"
#include "ui/ui.hpp"
#include "ui/ui_task.hpp"
#include "window/window.hpp"
#include "window/window_event.hpp"
#include "window/window_task.hpp"

namespace violet::editor
{
editor::editor() : core::system_base("editor")
{
}

bool editor::initialize(const dictionary& config)
{
    m_ui = std::make_unique<editor_ui>();

    initialize_task();
    initialize_camera();

    system<core::event>().subscribe<window::event_window_resize>(
        "editor",
        [this](std::uint32_t width, std::uint32_t height) { resize(width, height); });

    return true;
}

void editor::initialize_task()
{
    auto& task = system<task::task_manager>();

    auto editor_tick_task = task.schedule(TASK_EDITOR_TICK, [this]() { m_ui->tick(); });
    editor_tick_task->add_dependency(*task.find(task::TASK_GAME_LOGIC_END));

    auto ui_tick_task = task.find(ui::TASK_UI_TICK);
    ui_tick_task->add_dependency(*editor_tick_task);
}

void editor::initialize_camera()
{
    auto& world = system<ecs::world>();
    auto& graphics = system<graphics::graphics>();
    auto& window = system<window::window>();

    m_editor_camera = world.create("editor_camera");
    world.add<graphics::camera, scene::transform>(m_editor_camera);

    auto& transform = world.component<scene::transform>(m_editor_camera);
    transform.position(math::float3{0.0f, 11.0f, -60.0f});

    auto& camera = world.component<graphics::camera>(m_editor_camera);
    camera.render_groups = graphics::RENDER_GROUP_EDITOR | graphics::RENDER_GROUP_UI;

    auto extent = window.extent();
    resize(extent.width, extent.height);

    graphics.editor_camera(m_editor_camera, m_ui->scene_camera());
}

void editor::resize(std::uint32_t width, std::uint32_t height)
{
    auto& world = system<ecs::world>();

    auto& camera = world.component<graphics::camera>(m_editor_camera);

    graphics::render_target_desc render_target = {};
    render_target.format = graphics::rhi::back_buffer_format();
    render_target.width = width;
    render_target.height = height;
    render_target.samples = 4;
    m_render_target = graphics::rhi::make_render_target(render_target);
    camera.render_target(m_render_target.get());

    graphics::depth_stencil_buffer_desc depth_stencil_buffer = {};
    depth_stencil_buffer.format = graphics::RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_buffer.width = width;
    depth_stencil_buffer.height = height;
    depth_stencil_buffer.samples = 4;
    m_depth_stencil_buffer = graphics::rhi::make_depth_stencil_buffer(depth_stencil_buffer);
    camera.depth_stencil_buffer(m_depth_stencil_buffer.get());
}
}; // namespace violet::editor