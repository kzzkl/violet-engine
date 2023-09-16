#include "scene/scene_system.hpp"
#include "components/transform.hpp"
#include "core/engine.hpp"
#include "core/node/node.hpp"

namespace violet
{
scene_system::scene_system() : engine_system("scene")
{
}

bool scene_system::initialize(const dictionary& config)
{
    engine::on_frame_end().then(
        []()
        {
            view<transform> view(engine::get_world());
            view.each(
                [](transform& transform)
                {
                });
        });

    return true;
}
} // namespace violet