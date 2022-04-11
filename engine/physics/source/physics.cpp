#include "physics.hpp"
#include "scene.hpp"

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

    auto& task = system<ash::task::task_manager>();
    task.schedule(TASK_SIMULATION, [this]() { simulation(); });

    system<ash::ecs::world>().register_component<rigidbody>();
    system<ash::ecs::world>().register_component<joint>();
    m_rigidbody_view = system<ash::ecs::world>().make_view<rigidbody, ash::scene::transform>();
    m_joint_view = system<ash::ecs::world>().make_view<rigidbody, joint>();

    return true;
}

void physics::simulation()
{
    system<ash::scene::scene>().reset_sync_counter();
    system<ash::scene::scene>().sync_local();

    m_rigidbody_view->each([this](rigidbody& rigidbody, ash::scene::transform& transform) {
        rigidbody.node(transform.node.get());
        if (rigidbody.interface == nullptr)
        {
            rigidbody_desc desc = {};
            desc.type = rigidbody.type();
            desc.mass = rigidbody.mass();
            desc.linear_dimmer = rigidbody.linear_dimmer();
            desc.angular_dimmer = rigidbody.angular_dimmer();
            desc.restitution = rigidbody.restitution();
            desc.friction = rigidbody.friction();
            desc.shape = rigidbody.shape();
            desc.reflect = rigidbody.reflect();

            rigidbody.interface.reset(m_factory->make_rigidbody(desc));

            m_world->add(
                rigidbody.interface.get(),
                rigidbody.collision_group(),
                rigidbody.collision_mask());
        }
    });

    m_joint_view->each([this](rigidbody& rigidbody, joint& joint) {
        for (auto& unit : joint)
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
            desc.rigidbody_a = rigidbody.interface.get();
            desc.rigidbody_b = unit.rigidbody->interface.get();

            unit.interface.reset(m_factory->make_joint(desc));

            m_world->add(unit.interface.get());
        }
    });

    m_world->simulation(system<ash::core::timer>().frame_delta());

    system<ash::scene::scene>().sync_world();

#if defined(ASH_PHYSICS_DEBUG_DRAW)
    m_world->debug();
#endif
}
} // namespace ash::physics