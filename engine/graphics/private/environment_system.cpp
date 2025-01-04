#include "environment_system.hpp"
#include "components/scene_component.hpp"
#include "components/skybox_component.hpp"

namespace violet
{
environment_system::environment_system()
    : system("environment")
{
}

bool environment_system::initialize(const dictionary& config)
{
    auto& world = get_world();
    world.register_component<skybox_component>();

    return true;
}

void environment_system::update(render_scene_manager& scene_manager)
{
    auto& world = get_world();

    world.get_view().read<skybox_component>().read<scene_component>().each(
        [&](const skybox_component& skybox, const scene_component& scene)
        {
            render_scene* render_scene = scene_manager.get_scene(scene.layer);
            render_scene->set_skybox(skybox.texture, skybox.irradiance, skybox.prefilter);
        },
        [this](auto& view)
        {
            return view.template is_updated<skybox_component>(m_system_version);
        });

    m_system_version = world.get_version();
}
} // namespace violet