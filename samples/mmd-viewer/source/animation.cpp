#include "animation.hpp"

namespace ash::sample::mmd
{
bool animation::initialize(const ash::dictionary& config)
{
    auto& world = system<ash::ecs::world>();
    world.register_component<skeleton>();
    world.register_component<bone>();

    m_view = world.make_view<skeleton>();
    m_bone_view = world.make_view<bone, scene::transform>();

    return true;
}

void animation::update()
{
    auto& world = system<ecs::world>();

    m_bone_view->each([](bone& bone, scene::transform& transform) {
        math::float4x4_simd to_world = math::simd::load(transform.world_matrix);
        math::float4x4_simd offset = math::simd::load(bone.offset);
        math::simd::store(math::matrix_simd::mul(offset, to_world), bone.transform);
    });

    m_view->each([&](skeleton& skeleton) {
        for (std::size_t i = 0; i < skeleton.nodes.size(); ++i)
            skeleton.transform[i] = world.component<bone>(skeleton.nodes[i]).transform;

        skeleton.parameter->set(0, skeleton.transform.data(), skeleton.transform.size());
    });
}
} // namespace ash::sample::mmd