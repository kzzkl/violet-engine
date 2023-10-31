#include "common/log.hpp"
#include "control/control_system.hpp"
#include "core/engine.hpp"
#include "graphics/graphics_system.hpp"
#include "mmd_viewer.hpp"
#include "physics/physics_system.hpp"
#include "scene/scene_system.hpp"
#include "window/window_system.hpp"

int main()
{
    using namespace violet;

    engine engine;

    engine.initialize("mmd-viewer/config");
    engine.install<window_system>();
    engine.install<scene_system>();
    engine.install<graphics_system>();
    engine.install<physics_system>();
    engine.install<control_system>();
    engine.install<sample::mmd_viewer>();

    engine.get_system<window_system>().on_destroy().then(
        [&engine]()
        {
            log::info("Close window");
            engine.exit();
        });

    engine.run();

    return 0;
}