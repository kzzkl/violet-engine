#include "physics.hpp"
#include "graphics.hpp"
#include "scene.hpp"

namespace ash::physics
{
class physics_debug : public debug_draw_interface
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

physics::physics() noexcept : submodule("physics")
{
}

physics::~physics()
{
}

bool physics::initialize(const dictionary& config)
{
    m_plugin.load("ash-physics-bullet3.dll");

    m_debug = std::make_unique<physics_debug>(&module<ash::graphics::graphics>().debug());
    m_plugin.debug(m_debug.get());

    m_factory = m_plugin.factory();

    world_desc desc;
    desc.gravity = {0.0f, -10.0f, 0.0f};
    m_world.reset(m_factory->make_world(desc));

    auto& task = module<ash::task::task_manager>();
    task.schedule(TASK_SIMULATION, [this]() { simulation(); });

    module<ash::ecs::world>().register_component<rigidbody>();
    m_view = module<ash::ecs::world>().make_view<rigidbody, ash::scene::transform>();

    return true;
}

void physics::simulation()
{
    module<ash::scene::scene>().reset_sync_counter();
    module<ash::scene::scene>().sync_local();

    m_view->each([this](rigidbody& rigidbody, ash::scene::transform& transform) {
        if (rigidbody.in_world() == transform.node()->in_scene())
            return;

        if (transform.node()->in_scene())
        {
            if (rigidbody.interface == nullptr)
            {
                rigidbody_desc desc = {};
                desc.mass = rigidbody.mass();
                desc.shape = rigidbody.shape();
                desc.world_matrix = transform.node()->to_world;
                rigidbody.interface.reset(m_factory->make_rigidbody(desc));
            }

            m_world->add(rigidbody.interface.get());
        }
        else
        {
            m_world->remove(rigidbody.interface.get());
        }

        rigidbody.in_world(transform.node()->in_scene());
    });

    m_world->simulation(module<ash::core::timer>().frame_delta());

    m_view->each([this](rigidbody& rigidbody, ash::scene::transform& transform) {
        if (!rigidbody.in_world())
            return;

        if (rigidbody.interface->updated())
            transform.node()->to_world = rigidbody.interface->transform();
    });

    module<ash::scene::scene>().sync_world();
}
} // namespace ash::physics