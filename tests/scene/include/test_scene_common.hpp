#pragma once

#include "components/hierarchy_component.hpp"
#include "components/transform_component.hpp"
#include "core/engine.hpp"
#include "ecs/world.hpp"
#include "math/quaternion.hpp"
#include "scene/transform_system.hpp"
#include "task/task_executor.hpp"
#include "task/task_graph.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace violet::test
{
extern world* g_world;
extern transform_system* g_transform_system;
extern task_graph* g_task_graph;
extern task_executor* g_task_executor;

// Run one full frame tick: processes hierarchy changes, then updates local and world matrices.
inline void tick()
{
    g_task_executor->execute_sync(*g_task_graph);
    g_world->add_version();
}

// Force-update local and world matrices without hierarchy management.
inline void update_transform()
{
    g_transform_system->update_transform();
}

inline vec3f get_world_pos(entity e)
{
    return g_world->get_component<const transform_world_component>(e).get_position();
}

inline vec3f get_world_scale(entity e)
{
    return g_world->get_component<const transform_world_component>(e).scale;
}

inline bool approx_eq(float a, float b, float eps = 1e-4f)
{
    return std::abs(a - b) <= eps;
}

inline bool vec3_approx_eq(vec3f a, vec3f b, float eps = 1e-4f)
{
    return approx_eq(a.x, b.x, eps) && approx_eq(a.y, b.y, eps) && approx_eq(a.z, b.z, eps);
}
} // namespace violet::test