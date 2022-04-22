#include "editor.hpp"
#include "editor_layout.hpp"
#include "graphics.hpp"
#include "relation.hpp"
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
    return true;
}

void editor::initialize_task()
{
    auto& task = system<task::task_manager>();

    auto window_task = task.schedule(
        "window tick",
        [this]() { system<window::window>().tick(); },
        task::task_type::MAIN_THREAD);
    auto render_task = task.schedule("render", [this]() { system<graphics::graphics>().render(); });

    auto draw_ui_task = task.schedule("draw ui", [this]() {
        test_update();
        draw();
    });

    window_task->add_dependency(*task.find("root"));
    draw_ui_task->add_dependency(*window_task);
    render_task->add_dependency(*draw_ui_task);
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

    static int index = 0;

    if (window.keyboard().key(window::keyboard_key::KEY_1).release())
    {
        ecs::entity cube = world.create("cube" + std::to_string(index++));
        world.add<core::link>(cube);
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
    m_app.install<ui::ui>();
    m_app.install<editor_layout>();
    m_app.install<editor>();
}

void editor_app::run()
{
    m_app.run();
}
}; // namespace ash::editor