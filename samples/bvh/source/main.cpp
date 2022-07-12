#include "bvh_viewer.hpp"
#include "core/application.hpp"
#include "core/relation.hpp"
#include "graphics/graphics.hpp"
#include "scene/scene.hpp"
#include "ui/ui.hpp"
#include "window/window.hpp"

int main()
{
    ash::core::application app;
    app.install<ash::window::window>();
    app.install<ash::core::relation>();
    app.install<ash::scene::scene>();
    app.install<ash::graphics::graphics>();
    app.install<ash::ui::ui>();
    app.install<ash::sample::bvh_viewer>();

    app.run();

    return 0;
}