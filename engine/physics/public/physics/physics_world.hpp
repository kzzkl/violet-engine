#pragma once

#include "core/ecs/actor.hpp"
#include "physics/physics_context.hpp"

namespace violet
{
class physics_world
{
public:
    physics_world(const float3& gravity, phy_debug_draw* debug, physics_context* context);
    physics_world(const physics_world&) = delete;
    ~physics_world();

    void add(actor* actor);

    void simulation(float time_step);

    physics_world& operator=(const physics_world&) = delete;

private:
    phy_ptr<phy_world> m_world;
};
} // namespace violet