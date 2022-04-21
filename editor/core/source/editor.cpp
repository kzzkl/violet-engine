#include "editor.hpp"
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

    window_task->add_dependency(*task.find("root"));
    render_task->add_dependency(*window_task);
}

void editor::draw()
{
    
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
    m_app.install<editor>();
}

void editor_app::run()
{
    m_app.run();
}
}; // namespace ash::editor