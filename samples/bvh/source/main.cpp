#include "bvh_viewer.hpp"
#include "core/application.hpp"
#include "core/relation.hpp"
#include "graphics/graphics.hpp"
#include "scene/scene.hpp"
#include "ui/ui.hpp"
#include "window/window.hpp"

int main()
{
    violet::core::application app;
    app.install<violet::window::window>();
    app.install<violet::core::relation>();
    app.install<violet::scene::scene>();
    app.install<violet::graphics::graphics>();
    app.install<violet::ui::ui>();
    app.install<violet::sample::bvh_viewer>();

    app.run();

    return 0;
}