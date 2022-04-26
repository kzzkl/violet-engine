#include "physics.hpp"
#include "relation.hpp"
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
    physics_debug(ash::graphics::graphics_debug* drawer) : m_drawer(drawer) {}

    virtual void draw_line(
        const math::float3& start,
        const math::float3& end,
        const math::float3& color) override
    {
        m_drawer->draw_line(start, end, color);
    }

private:
    ash::graphics::graphics_debug* m_drawer;
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

    world_desc desc = {};
    desc.gravity = {config["gravity"][0], config["gravity"][1], config["gravity"][2]};
    m_world.reset(m_factory->make_world(desc));

    auto& world = system<ash::ecs::world>();
    world.register_component<rigidbody>();
    world.register_component<joint>();
    m_view = world.make_view<rigidbody>();

    auto& event = system<core::event>();
    event.subscribe<scene::event_enter_scene>(
        [this](ecs::entity entity) { m_initialize_list.push_back(entity); });

    return true;
}

void physics::simulation()
{
    auto& world = system<ecs::world>();

    system<ash::scene::scene>().sync_local();

    if (!m_initialize_list.empty())
    {
        for (auto& entity : m_initialize_list)
            initialize_entity(entity);
        m_initialize_list.clear();
    }

    m_view->each([&](rigidbody& rigidbody) {
        auto& transform = world.component<scene::transform>(rigidbody.relation);
        if (transform.sync_count != 0 && rigidbody.type == rigidbody_type::KINEMATIC)
        {
            math::float4x4_simd to_world = math::simd::load(transform.world_matrix);
            math::float4x4_simd offset = math::simd::load(rigidbody.offset);

            math::float4x4 rigidbody_to_world;
            math::simd::store(math::matrix_simd::mul(offset, to_world), rigidbody_to_world);
            rigidbody.interface->transform(rigidbody_to_world);
        }
    });

    m_world->simulation(system<ash::core::timer>().frame_delta());

    while (true)
    {
        rigidbody_interface* updated = m_world->updated_rigidbody();
        if (updated == nullptr)
            break;

        ecs::entity entity = m_user_data[updated->user_data_index].entity;
        auto& r = world.component<rigidbody>(entity);
        auto& t = world.component<scene::transform>(r.relation);

        math::float4x4_simd to_world = math::simd::load(updated->transform());
        math::float4x4_simd offset_inverse = math::simd::load(r.offset_inverse);
        math::simd::store(math::matrix_simd::mul(offset_inverse, to_world), t.world_matrix);
        t.dirty = true;
    }

    system<ash::scene::scene>().sync_world();

#if defined(ASH_PHYSICS_DEBUG_DRAW)
    m_world->debug();
#endif
}

void physics::initialize_entity(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    auto init_rigidbody = [&, this](ecs::entity node) {
        if (!world.has_component<rigidbody>(node))
            return;

        auto& r = world.component<rigidbody>(node);
        auto& transform = world.component<scene::transform>(r.relation);

        if (r.interface == nullptr)
        {
            rigidbody_desc desc = {};
            desc.type = r.type;
            desc.mass = r.mass;
            desc.linear_dimmer = r.linear_dimmer;
            desc.angular_dimmer = r.angular_dimmer;
            desc.restitution = r.restitution;
            desc.friction = r.friction;
            desc.shape = r.shape;
            math::float4x4_simd to_world = math::simd::load(transform.world_matrix);
            math::float4x4_simd offset = math::simd::load(r.offset);
            math::simd::store(math::matrix_simd::mul(offset, to_world), desc.initial_transform);
            math::simd::store(math::matrix_simd::inverse(offset), r.offset_inverse);

            r.interface.reset(m_factory->make_rigidbody(desc));
            r.interface->user_data_index = m_user_data.size();
            m_user_data.push_back({node});

            m_world->add(r.interface.get(), r.collision_group, r.collision_mask);
        }
    };

    auto init_joint = [&, this](ecs::entity node) {
        if (!world.has_component<joint>(node))
            return;

        auto& j = world.component<joint>(node);
        if (j.interface != nullptr)
            return;

        joint_desc desc = {};
        desc.location = j.location;
        desc.rotation = j.rotation;
        desc.min_linear = j.min_linear;
        desc.max_linear = j.max_linear;
        desc.min_angular = j.min_angular;
        desc.max_angular = j.max_angular;
        desc.spring_translate_factor = j.spring_translate_factor;
        desc.spring_rotate_factor = j.spring_rotate_factor;

        desc.rigidbody_a = world.component<rigidbody>(j.relation_a).interface.get();
        desc.rigidbody_b = world.component<rigidbody>(j.relation_b).interface.get();

        j.interface.reset(m_factory->make_joint(desc));

        m_world->add(j.interface.get());
    };

    relation.each_bfs(entity, init_rigidbody);
    relation.each_bfs(entity, init_joint);
}
} // namespace ash::physics