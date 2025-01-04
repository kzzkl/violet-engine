#include "scene/scene_system.hpp"
#include "components/scene_component.hpp"
#include "scene/hierarchy_system.hpp"
#include "scene/transform_system.hpp"

namespace violet
{
scene_system::scene_system()
    : system("scene")
{
}

void scene_system::install(application& app)
{
    app.install<hierarchy_system>();
    app.install<transform_system>();
}

bool scene_system::initialize(const dictionary& config)
{
    get_world().register_component<scene_component>();
    return true;
}
} // namespace violet