#include "mmd_viewer.hpp"
#include "core/timer.hpp"
#include "graphics/rhi.hpp"
#include "mmd_animation.hpp"
#include "scene/scene.hpp"
#include "scene/transform.hpp"
#include "window/window_event.hpp"

namespace ash::sample::mmd
{
bool mmd_viewer::initialize(const dictionary& config)
{
    auto& world = system<ash::ecs::world>();
    world.register_component<mmd_node>();
    world.register_component<mmd_skeleton>();

    m_loader = std::make_unique<mmd_loader>();
    m_loader->initialize();

    graphics::rhi::register_pipeline_parameter_layout(
        "mmd_material",
        material_pipeline_parameter::layout());
    graphics::rhi::register_pipeline_parameter_layout(
        "mmd_skin",
        skin_pipeline_parameter::layout());

    m_render_pipeline = std::make_unique<mmd_render_pipeline>();
    m_skin_pipeline = std::make_unique<mmd_skin_pipeline>();

    return true;
}

ash::ecs::entity mmd_viewer::load_mmd(
    std::string_view name,
    std::string_view pmx,
    std::string_view vmd)
{
    ecs::entity entity = system<ecs::world>().create(name);
    mmd_resource resource;
    if (m_loader->load(entity, resource, pmx, vmd, m_render_pipeline.get(), m_skin_pipeline.get()))
    {
        m_resources[name.data()] = std::move(resource);
        return entity;
    }
    else
    {
        system<ecs::world>().release(entity);
        return ecs::INVALID_ENTITY;
    }
}

void mmd_viewer::update()
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& animation = system<mmd_animation>();

    world.view<mmd_skeleton, graphics::skinned_mesh>().each(
        [&](ecs::entity entity, mmd_skeleton& skeleton, graphics::skinned_mesh&) {
            reset(entity);
        });

    static float delta = 0.0f;
    delta += system<core::timer>().frame_delta();
    animation.evaluate(delta * 30.0f);
    animation.update(false);
    world.view<mmd_skeleton, graphics::skinned_mesh>().each(
        [&](mmd_skeleton& skeleton, graphics::skinned_mesh&) {
            math::float4_simd scale;
            math::float4_simd rotation;
            math::float4_simd position;
            for (std::size_t i = 0; i < skeleton.nodes.size(); ++i)
            {
                auto& transform = world.component<scene::transform>(skeleton.nodes[i]);
                math::matrix_simd::decompose(
                    math::simd::load(skeleton.local[i]),
                    scale,
                    rotation,
                    position);
                transform.scale(scale);
                transform.rotation(rotation);
                transform.position(position);
            }
        });

    // system<physics::physics>().simulation();

    animation.update(true);

    world.view<mmd_skeleton, graphics::skinned_mesh>().each(
        [&](mmd_skeleton& skeleton, graphics::skinned_mesh& skinned_mesh) {
            math::float4x4_simd to_world;
            math::float4x4_simd initial_inverse;
            math::float4x4_simd final_transform;
            for (std::size_t i = 0; i < skeleton.nodes.size(); ++i)
            {
                auto& transform = world.component<scene::transform>(skeleton.nodes[i]);
                auto& node = world.component<mmd_node>(skeleton.nodes[i]);

                to_world = math::simd::load(transform.to_world());
                initial_inverse = math::simd::load(node.initial_inverse);
                final_transform = math::matrix_simd::mul(initial_inverse, to_world);
                math::simd::store(final_transform, skeleton.world[i]);
            }

            auto parameter = dynamic_cast<skin_pipeline_parameter*>(skinned_mesh.parameter.get());
            ASH_ASSERT(parameter);
            parameter->bone_transform(skeleton.world);
        });
}

void mmd_viewer::reset(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& skeleton = world.component<mmd_skeleton>(entity);

    for (auto& node_entity : skeleton.nodes)
    {
        auto& node = world.component<mmd_node>(node_entity);
        auto& node_transform = world.component<scene::transform>(node_entity);
        node_transform.position(node.initial_position);
        node_transform.rotation(node.initial_rotation);
        node_transform.scale(node.initial_scale);

        node.inherit_translate = {0.0f, 0.0f, 0.0f};
        node.inherit_rotate = {0.0f, 0.0f, 0.0f, 1.0f};
    }
}
} // namespace ash::sample::mmd