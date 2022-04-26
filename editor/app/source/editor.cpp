#include "editor.hpp"
#include "editor_layout.hpp"
#include "graphics.hpp"
#include "relation.hpp"
#include "sample.hpp"
#include "scene.hpp"
#include "ui.hpp"
#include "window.hpp"

namespace ash::editor
{
editor::editor() : core::system_base("editor")
{
}

bool editor::initialize(const dictionary& config)
{
    initialize_task();
    initialize_camera();
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
    // auto render_task = task.schedule("render", [this]() {});

    auto draw_ui_task = task.schedule("draw ui", [&, this]() {
        auto& graphics = system<graphics::graphics>();
        test_update();
        scene.sync_local();

        system<physics::physics>().simulation();

        graphics.begin_frame();
        draw();
        graphics.render(m_editor_camera);
        graphics.end_frame();
    });

    window_task->add_dependency(*task.find("root"));
    draw_ui_task->add_dependency(*window_task);
    // render_task->add_dependency(*draw_ui_task);
}

void editor::initialize_camera()
{
    auto& world = system<ecs::world>();
    auto& relation = system<core::relation>();
    auto& graphics = system<graphics::graphics>();
    auto& scene = system<scene::scene>();

    m_editor_camera = world.create("editor_camera");
    world.add<graphics::camera, scene::transform>(m_editor_camera);

    auto& transform = world.component<scene::transform>(m_editor_camera);
    transform.position = {0.0f, 11.0f, -60.0f};
    transform.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    transform.scaling = {1.0f, 1.0f, 1.0f};

    auto& camera = world.component<graphics::camera>(m_editor_camera);
    camera.set(math::to_radians(30.0f), 1300.0f / 800.0f, 0.01f, 1000.0f);
    camera.parameter = graphics.make_render_parameter("ash_pass");
    camera.mask = graphics::visual::mask_type::EDITOR | graphics::visual::mask_type::UI;
}

void editor::draw()
{
    auto& ui = system<ui::ui>();

    ui.begin_frame();
    system<editor_layout>().draw();
    ui.end_frame();
}

void editor::test_update()
{
    auto& window = system<window::window>();
    auto& world = system<ecs::world>();
    auto& relation = system<core::relation>();
    auto& scene = system<scene::scene>();
    auto& graphics = system<graphics::graphics>();

    static int index = 0;

    if (window.keyboard().key(window::keyboard_key::KEY_1).release())
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
    m_app.install<editor_layout>();
    m_app.install<editor>();
    m_app.install<test_module>();
}

void editor_app::run()
{
    m_app.run();
}
}; // namespace ash::editor