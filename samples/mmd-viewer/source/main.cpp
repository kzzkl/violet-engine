#include "control/control_system.hpp"
#include "core/engine.hpp"
#include "ecs_command/ecs_command_system.hpp"
#include "graphics/graphics_system.hpp"
#include "mmd_animation.hpp"
#include "mmd_viewer.hpp"
#include "physics/physics_system.hpp"
#include "scene/hierarchy_system.hpp"
#include "scene/scene_system.hpp"
#include "scene/transform_system.hpp"
#include "window/window_system.hpp"

int main()
{
    // https://github.com/benikabocha/saba

    violet::engine::initialize("mmd-viewer/config");
    violet::engine::install<violet::ecs_command_system>();
    violet::engine::install<violet::hierarchy_system>();
    violet::engine::install<violet::transform_system>();
    violet::engine::install<violet::scene_system>();
    violet::engine::install<violet::window_system>();
    violet::engine::install<violet::graphics_system>();
    violet::engine::install<violet::physics_system>();
    violet::engine::install<violet::control_system>();
    violet::engine::install<violet::sample::mmd_animation>();
    violet::engine::install<violet::sample::mmd_viewer>();

    violet::engine::run();

    return 0;
}