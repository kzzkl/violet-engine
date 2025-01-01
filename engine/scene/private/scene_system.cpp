#include "scene/scene_system.hpp"
#include "components/scene_component.hpp"

namespace violet
{
scene_system::scene_system()
    : engine_system("scene")
{
}

bool scene_system::initialize(const dictionary& config)
{
    get_world().register_component<scene_component>();
    return true;
}
} // namespace violet