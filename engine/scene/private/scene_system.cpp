#include "scene/scene_system.hpp"
#include "components/hierarchy.hpp"
#include "core/engine.hpp"

namespace violet
{
scene_system::scene_system()
    : engine_system("Scene")
{
}

bool scene_system::initialize(const dictionary& config)
{
    // get_world().register_component<hierarchy>();
    return true;
}
} // namespace violet