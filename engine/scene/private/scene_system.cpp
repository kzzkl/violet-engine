#include "scene/scene_system.hpp"
#include "components/hierarchy.hpp"
#include "components/scene_layer.hpp"

namespace violet
{
scene_system::scene_system()
    : engine_system("scene")
{
}

bool scene_system::initialize(const dictionary& config)
{
    get_world().register_component<scene_layer>();
    return true;
}

scene* scene_system::create_scene(const std::string& name)
{
    m_scenes.push_back(std::make_unique<scene>(name, static_cast<std::uint32_t>(m_scenes.size())));
    return m_scenes.back().get();
}
} // namespace violet