#pragma once

#include "components/rigidbody.hpp"
#include "core/node/node.hpp"
#include "physics/physics_interface.hpp"

namespace violet
{
class physics_world
{
public:
    physics_world(const float3& gravity, pei_debug_draw* debug, pei_plugin* pei);
    physics_world(const physics_world&) = delete;
    ~physics_world();

    void add(component_ptr<rigidbody> rigidbody);

    void simulation(float time_step);

    pei_plugin* get_pei() const noexcept { return m_pei; }

    physics_world& operator=(const physics_world&) = delete;

private:
    pei_world* m_world;

    pei_plugin* m_pei;
};
} // namespace violet