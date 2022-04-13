#include "physics.hpp"
#include "scene.hpp"
#include "scene_event.hpp"

#if defined(ASH_PHYSICS_DEBUG_DRAW)
#    include "graphics.hpp"
#endif

namespace ash::physics
{
#if defined(ASH_PHYSICS_DEBUG_DRAW)
class physics::physics_debug : public debug_draw_interface
{
public:
    physics_debug(ash::graphics::debug_pipeline* drawer) : m_drawer(drawer) {}

    virtual void draw_line(
        const math::float3& start,
        const math::float3& end,
        const math::float3& color) override
    {
        m_drawer->draw_line(start, end, color);
    }

private:
    ash::graphics::debug_pipeline* m_drawer;
};
#endif

physics::physics() noexcept : system_base("physics")
{
}

physics::~physics()
{
}

bool physics::initialize(const dictionary& config)
{
    m_plugin.load("ash-physics-bullet3.dll");

#if defined(ASH_PHYSICS_DEBUG_DRAW)
    m_debug = std::make_unique<physics::physics_debug>(&system<ash::graphics::graphics>().debug());
    m_plugin.debug(m_debug.get());
#endif

    m_factory = m_plugin.factory();

    world_desc desc;
    desc.gravity = {0.0f, -10.0f, 0.0f};
    m_world.reset(m_factory->make_world(desc));

    auto& world = system<ash::ecs::world>();
    world.register_component<rigidbody>();
    world.register_component<joint>();
    m_view = world.make_view<rigidbody, scene::transform>();

    auto& event = system<core::event>();
    event.subscribe<scene::event_enter_scene>(
        [this](ecs::entity entity) { on_enter_scene(entity); });

    return true;
}

void physics::simulation()
{
    system<ash::scene::scene>().sync_local();

    m_view->each([](rigidbody& rigidbody, scene::transform& transform) {
        if (transform.sync_count != 0 && rigidbody.type() == rigidbody_type::KINEMATIC)
        {
            math::float4x4_simd to_world = math::simd::load(transform.world_matrix);
            math::float4x4_simd offset = math::simd::load(rigidbody.offset());

            math::float4x4 rigidbody_to_world;
            math::simd::store(math::matrix_simd::mul(offset, to_world), rigidbody_to_world);
            rigidbody.interface->transform(rigidbody_to_world);
        }
    });

    m_world->simulation(system<ash::core::timer>().frame_delta());

    auto& world = system<ecs::world>();
    while (true)
    {
        rigidbody_interface* updated = m_world->updated_rigidbody();
        if (updated == nullptr)
            break;

        ecs::entity entity = m_user_data[updated->user_data_index].entity;
        auto& r = world.component<rigidbody>(entity);
        auto& t = world.component<scene::transform>(entity);

        math::float4x4_simd to_world = math::simd::load(updated->transform());
        math::float4x4_simd offset_inverse = math::simd::load(r.offset_inverse());
        math::simd::store(math::matrix_simd::mul(offset_inverse, to_world), t.world_matrix);
        t.dirty = true;
    }

    system<ash::scene::scene>().sync_world();

#if defined(ASH_PHYSICS_DEBUG_DRAW)
    m_world->debug();
#endif
}

void physics::on_enter_scene(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();

    auto init_rigidbody = [&, this](ecs::entity node) {
        if (!world.has_component<rigidbody>(node))
            return;

        auto& r = world.component<rigidbody>(node);
        auto& t = world.component<scene::transform>(node);

        r.node(node);
        if (r.interface == nullptr)
        {
            rigidbody_desc desc = {};
            desc.type = r.type();
            desc.mass = r.mass();
            desc.linear_dimmer = r.linear_dimmer();
            desc.angular_dimmer = r.angular_dimmer();
            desc.restitution = r.restitution();
            desc.friction = r.friction();
            desc.shape = r.shape();
            math::float4x4_simd to_world = math::simd::load(t.world_matrix);
            math::float4x4_simd offset = math::simd::load(r.offset());
            math::simd::store(math::matrix_simd::mul(offset, to_world), desc.initial_transform);

            r.interface.reset(m_factory->make_rigidbody(desc));
            r.interface->user_data_index = m_user_data.size();
            m_user_data.push_back({node});

            m_world->add(r.interface.get(), r.collision_group(), r.collision_mask());
        }
    };

    auto init_joint = [&, this](ecs::entity node) {
        if (!world.has_component<joint>(node))
            return;

        for (auto& unit : world.component<joint>(node))
        {
            if (unit.interface != nullptr)
                continue;

            joint_desc desc = {};
            desc.location = unit.location;
            desc.rotation = unit.rotation;
            desc.min_linear = unit.min_linear;
            desc.max_linear = unit.max_linear;
            desc.min_angular = unit.min_angular;
            desc.max_angular = unit.max_angular;
            desc.spring_translate_factor = unit.spring_translate_factor;
            desc.spring_rotate_factor = unit.spring_rotate_factor;

            desc.rigidbody_a = world.component<rigidbody>(node).interface.get();
            desc.rigidbody_b = world.component<rigidbody>(unit.entity).interface.get();

            unit.interface.reset(m_factory->make_joint(desc));

            m_world->add(unit.interface.get());
        }
    };

    scene.each_children(entity, init_rigidbody);
    scene.each_children(entity, init_joint);
}
} // namespace ash::physics