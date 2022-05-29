#include "editor/editor.hpp"
#include "core/relation.hpp"
#include "editor/sample.hpp"
#include "graphics/graphics.hpp"
#include "scene/scene.hpp"
#include "ui/ui.hpp"
#include "window/window.hpp"
#include "window/window_event.hpp"

namespace ash::editor
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
    auto& scene = system<scene::scene>();

    auto window_task = task.schedule(
        "window tick",
        [this]() { system<window::window>().tick(); },
        task::task_type::MAIN_THREAD);

    auto draw_ui_task = task.schedule("draw ui", [&, this]() {
        auto& graphics = system<graphics::graphics>();
        auto& ui = system<ui::ui>();

        test_update();

        system<physics::physics>().simulation();

        m_ui->tick();
        ui.tick();
        graphics.render(m_editor_camera);
        graphics.present();
    });

    window_task->add_dependency(*task.find("root"));
    draw_ui_task->add_dependency(*window_task);
}

void editor::initialize_camera()
{
    auto& world = system<ecs::world>();
    auto& relation = system<core::relation>();
    auto& graphics = system<graphics::graphics>();
    auto& scene = system<scene::scene>();

    m_editor_camera = world.create("editor_camera");
    world.add<graphics::camera, graphics::main_camera, scene::transform>(m_editor_camera);

    auto& transform = world.component<scene::transform>(m_editor_camera);
    transform.position = {0.0f, 11.0f, -60.0f};
    transform.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    transform.scaling = {1.0f, 1.0f, 1.0f};

    auto& camera = world.component<graphics::camera>(m_editor_camera);
    camera.parameter = graphics.make_pipeline_parameter("ash_pass");
    camera.mask = graphics::VISUAL_GROUP_EDITOR | graphics::VISUAL_GROUP_UI;
    resize(2000, 1200);
}

void editor::resize(std::uint32_t width, std::uint32_t height)
{
    auto& world = system<ecs::world>();
    auto& graphics = system<graphics::graphics>();

    auto& camera = world.component<graphics::camera>(m_editor_camera);
    camera.set(
        math::to_radians(45.0f),
        static_cast<float>(width) / static_cast<float>(height),
        0.3f,
        1000.0f);

    graphics::render_target_info render_target_info = {};
    render_target_info.format = graphics.back_buffer_format();
    render_target_info.width = width;
    render_target_info.height = height;
    render_target_info.samples = 4;
    m_render_target = graphics.make_render_target(render_target_info);
    camera.render_target = m_render_target.get();

    graphics::depth_stencil_buffer_info depth_stencil_buffer_info = {};
    depth_stencil_buffer_info.format = graphics::resource_format::D24_UNORM_S8_UINT;
    depth_stencil_buffer_info.width = width;
    depth_stencil_buffer_info.height = height;
    depth_stencil_buffer_info.samples = 4;
    m_depth_stencil_buffer = graphics.make_depth_stencil_buffer(depth_stencil_buffer_info);
    camera.depth_stencil_buffer = m_depth_stencil_buffer.get();
}

void editor::test_update()
{
    auto& window = system<window::window>();
    auto& world = system<ecs::world>();
    auto& relation = system<core::relation>();
    auto& scene = system<scene::scene>();
    auto& graphics = system<graphics::graphics>();

    static int index = 0;

    if (window.keyboard().key(window::KEYBOARD_KEY_1).release())
    {
        ecs::entity cube = world.create("cube" + std::to_string(index++));
        world.add<core::link, scene::transform>(cube);
        relation.link(cube, scene.root());
    }
}

editor_app::editor_app() : m_app("editor/config")
{
}

void editor_app::initialize()
{
    m_app.install<window::window>();
    m_app.install<core::relation>();
    m_app.install<scene::scene>();
    m_app.install<graphics::graphics>();
    m_app.install<physics::physics>();
    m_app.install<ui::ui>();
    m_app.install<editor>();
    m_app.install<test_module>();
}

void editor_app::run()
{
    m_app.run();
}
}; // namespace ash::editor