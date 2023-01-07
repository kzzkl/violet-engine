#include "core/application.hpp"
#include "core/relation.hpp"
#include "editor/editor.hpp"
#include "graphics/graphics.hpp"
#include "light_viewer.hpp"
#include "scene/scene.hpp"
#include "window/window.hpp"

int main()
{
    using namespace violet;

    core::application app;
    app.install<window::window>();
    app.install<core::relation>();
    app.install<scene::scene>();
    app.install<graphics::graphics>();
    // app.install<ui::ui>();
    // app.install<editor::editor>();
    app.install<sample::light_viewer>();

    app.run();

    return 0;
}