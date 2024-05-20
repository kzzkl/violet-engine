#include "scene/scene_module.hpp"
#include "components/transform.hpp"
#include "core/ecs/actor.hpp"

namespace violet
{
class transform_info : public component_info_default<transform>
{
public:
    virtual void construct(actor* owner, void* target) override { new (target) transform(owner); }
};

scene_module::scene_module() : engine_module("scene")
{
}

bool scene_module::initialize(const dictionary& config)
{
    get_world().register_component<transform, transform_info>();

    return true;
}
} // namespace violet