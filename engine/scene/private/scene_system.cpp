#include "scene/scene_system.hpp"
#include "components/transform.hpp"
#include "core/node/node.hpp"

namespace violet
{
scene_system::scene_system() : engine_system("scene")
{
}

bool scene_system::initialize(const dictionary& config)
{
    on_frame_end().then(
        [this]()
        {
            view<transform> view(get_world());
            view.each(
                [](transform& transform)
                {
                });
        });

    get_world().register_component<transform>();

    return true;
}
} // namespace violet