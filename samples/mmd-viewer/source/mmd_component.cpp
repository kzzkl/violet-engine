#include "mmd_component.hpp"
#include "ecs/world.hpp"
#include "graphics/mesh_render.hpp"
#include "scene/transform.hpp"

namespace violet::sample::mmd
{
void mmd_morph_controler::group_morph::evaluate(float weight, ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& controler = world.component<mmd_morph_controler>(entity);

    for (auto& d : data)
        controler.morphs[d.index]->evaluate(weight * d.weight, entity);
}

void mmd_morph_controler::vertex_morph::evaluate(float weight, ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& controler = world.component<mmd_morph_controler>(entity);

    for (auto& [index, translate] : data)
    {
        math::float3& target =
            *(static_cast<math::float3*>(controler.vertex_morph_result->pointer()) + index);

        math::float4_simd t1 = math::simd::load(translate);
        t1 = math::vector_simd::mul(t1, weight);
        math::float4_simd t2 = math::simd::load(target);
        t2 = math::vector_simd::add(t1, t2);
        math::simd::store(t2, target);
    }
}

void mmd_morph_controler::bone_morph::evaluate(float weight, ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& controler = world.component<mmd_morph_controler>(entity);
    auto& skeleton = world.component<mmd_skeleton>(entity);

    for (auto& d : data)
    {
        auto& transform = world.component<scene::transform>(skeleton.nodes[d.index]);

        math::float4_simd t = math::simd::load(d.translation);
        t = math::vector_simd::mul(t, weight);
        t = math::vector_simd::add(t, math::simd::load(transform.position()));
        transform.position(t);

        math::float4_simd r = math::simd::load(d.rotation);
        r = math::quaternion_simd::slerp(math::simd::load(transform.rotation()), r, weight);
        transform.rotation(r);
    }
}

void mmd_morph_controler::uv_morph::evaluate(float weight, ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& controler = world.component<mmd_morph_controler>(entity);

    for (auto& [index, uv] : data)
    {
        math::float2& target =
            *(static_cast<math::float2*>(controler.uv_morph_result->pointer()) + index);

        target[0] += weight * uv[0];
        target[1] += weight * uv[1];
    }
}

void mmd_morph_controler::material_morph::evaluate(float weight, ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& mesh_renderer = world.component<graphics::mesh_render>(entity);

    // TODO:
    for (auto& d : data)
    {
        if (d.index != -1)
        {
        }
    }
}
} // namespace violet::sample::mmd