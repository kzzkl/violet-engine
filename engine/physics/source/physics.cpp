#include "physics.hpp"
#include "graphics.hpp"
#include "scene.hpp"

namespace ash::physics
{
class physics_debug : public debug_draw_interface
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

    module<ash::ecs::world>().register_component<rigidbody, joint>();
    m_rigidbody_view = module<ash::ecs::world>().make_view<rigidbody, ash::scene::transform>();
    m_joint_view = module<ash::ecs::world>().make_view<rigidbody, joint>();

    return true;
}

void physics::simulation()
{
    module<ash::scene::scene>().reset_sync_counter();
    module<ash::scene::scene>().sync_local();

    m_rigidbody_view->each([this](rigidbody& rigidbody, ash::scene::transform& transform) {
        if (rigidbody.in_world() == transform.node()->in_scene())
            return;

        if (transform.node()->in_scene())
        {
            if (rigidbody.interface == nullptr)
            {
                rigidbody_desc desc = {};
                desc.mass = rigidbody.mass();
                desc.linear_dimmer = rigidbody.linear_dimmer();
                desc.angular_dimmer = rigidbody.angular_dimmer();
                desc.restitution = rigidbody.restitution();
                desc.friction = rigidbody.friction();
                desc.shape = rigidbody.shape();

                /*math::float4x4_simd to_world = math::simd::load(transform.node()->to_world);
                math::float4x4_simd to_node = math::simd::load(rigidbody.offset());

                math::float4x4_simd offset =
                    math::matrix_simd::mul(to_node, math::matrix_simd::inverse(to_world));
                rigidbody.offset(offset);*/
                desc.world_matrix = rigidbody.offset();

                // math::simd::store(to_node, desc.world_matrix);
                // math::simd::store(math::matrix_simd::mul(to_node, to_world), desc.world_matrix);

                rigidbody.interface.reset(m_factory->make_rigidbody(desc));
            }

            m_world->add(
                rigidbody.interface.get(),
                rigidbody.collision_group(),
                rigidbody.collision_mask());
        }
        else
        {
            m_world->remove(rigidbody.interface.get());
        }

        rigidbody.in_world(transform.node()->in_scene());
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

        /*if (!group.in_world)
        {
            for (auto& joint : group.joints)
            {
                joint_desc desc = {};
                desc.location = joint.location;
                desc.rotation = joint.rotation;
                desc.min_linear = joint.min_linear;
                desc.max_linear = joint.max_linear;
                desc.min_angular = joint.min_angular;
                desc.max_angular = joint.max_angular;
                desc.spring_translate_factor = joint.spring_translate_factor;
                desc.spring_rotate_factor = joint.spring_rotate_factor;
                desc.rigidbody_a = joint.rigidbody_a->interface.get();
                desc.rigidbody_b = joint.rigidbody_b->interface.get();

                joint.interface.reset(m_factory->make_joint(desc));

                m_world->add(joint.interface.get());
            }

            group.in_world = true;
        }*/
    });

    m_world->simulation(module<ash::core::timer>().frame_delta());

    m_rigidbody_view->each([this](rigidbody& rigidbody, ash::scene::transform& transform) {
        if (!rigidbody.in_world())
            return;

        // if (rigidbody.interface->updated())
        {
            math::float4x4_simd to_world = math::simd::load(rigidbody.interface->transform());
            math::float4x4_simd to_node_inv = math::simd::load(rigidbody.offset_inverse());
            // math::simd::store(to_world, transform.node()->to_world);

            math::simd::store(
                math::matrix_simd::mul(to_node_inv, to_world),
                transform.node()->to_world);
        }
    });

    module<ash::scene::scene>().sync_world();
}
} // namespace ash::physics