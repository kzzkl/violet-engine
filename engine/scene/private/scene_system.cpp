#include "scene/scene_system.hpp"
#include "components/transform.hpp"
#include "core/ecs/actor.hpp"

namespace violet
{
class transform_info : public component_info_default<transform>
{
public:
    virtual void construct(actor* owner, void* target) override { new (target) transform(owner); }
};

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

    get_world().register_component<transform, transform_info>();

    return true;
}
} // namespace violet