#pragma once

#include "mmd_component.hpp"
#include "world.hpp"

namespace ash::sample::mmd
{
class mmd_ik_solver
{
public:
    mmd_ik_solver(ecs::world& world);

    void solve(ecs::entity entity);

private:
    void solve_core(ecs::entity entity, std::uint32_t index);

    ecs::world& m_world;
};
} // namespace ash::sample::mmd