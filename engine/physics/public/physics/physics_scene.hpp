#pragma once

#include "physics/physics_context.hpp"

namespace violet
{
class physics_scene
{
public:
    physics_scene(const vec3f& gravity, phy_debug_draw* debug, physics_context* context);
    ~physics_scene();

    void add_rigidbody(phy_rigidbody* rigidbody);
    void remove_rigidbody(phy_rigidbody* rigidbody);

    void add_joint(phy_joint* joint);
    void remove_joint(phy_joint* joint);

    void simulation(float time_step);

private:
    phy_ptr<phy_world> m_world;
};
} // namespace violet