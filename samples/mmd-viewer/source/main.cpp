#include "common/log.hpp"
#include "control/control_module.hpp"
#include "core/engine.hpp"
#include "graphics/graphics_module.hpp"
#include "mmd_animation.hpp"
#include "mmd_viewer.hpp"
#include "physics/physics_module.hpp"
#include "scene/scene_module.hpp"
#include "window/window_module.hpp"

int main()
{
    using namespace violet;

    engine engine;

    engine.initialize("mmd-viewer/config");
    engine.install<window_module>();
    engine.install<scene_module>();
    engine.install<graphics_module>();
    engine.install<physics_module>();
    engine.install<control_module>();
    engine.install<sample::mmd_animation>();
    engine.install<sample::mmd_viewer>();

    engine.get_module<window_module>().on_destroy().then(
        [&engine]()
        {
            log::info("Close window");
            engine.exit();
        });

    engine.run();

    return 0;
}