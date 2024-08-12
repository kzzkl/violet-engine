#include "common/log.hpp"
#include "control/control_module.hpp"
#include "core/engine.hpp"
#include "graphics/graphics_module.hpp"
#include "mmd_animation.hpp"
#include "mmd_viewer.hpp"
#include "physics/physics_module.hpp"
#include "scene/scene_module.hpp"
#include "window/window_module.hpp"
#include <exception>

int main()
{
    try
    {
        violet::engine engine;

        engine.initialize("mmd-viewer/config");
        engine.install<violet::window_module>();
        engine.install<violet::scene_module>();
        engine.install<violet::graphics_module>();
        engine.install<violet::physics_module>();
        engine.install<violet::control_module>();
        engine.install<violet::sample::mmd_animation>();
        engine.install<violet::sample::mmd_viewer>();

        engine.get_module<violet::window_module>().on_destroy().then(
            [&engine]()
            {
                violet::log::info("Close window");
                engine.exit();
            });

        engine.run();
    }
    catch (const std::exception& e)
    {
        return -1;
    }

    return 0;
}