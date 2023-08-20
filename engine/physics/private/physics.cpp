#include "physics/physics.hpp"
#include "core/relation.hpp"
#include "core.hpp"
#include "physics_plugin.hpp"
#include "scene/scene.hpp"
#include "scene/scene_event.hpp"

#if defined(VIOLET_PHYSICS_DEBUG_DRAW)
#    include "graphics/graphics.hpp"
#endif

namespace violet::physics
{
#if defined(VIOLET_PHYSICS_DEBUG_DRAW)
class physics_debug : public debug_draw_interface
{
public:
    physics_debug() : m_drawer(nullptr) {}
    virtual ~physics_debug() {}

    static physics_debug& instance()
    {
        static physics_debug instance;
        return instance;
    }

    void initialize(graphics::graphics_debug* drawer) { m_drawer = drawer; }

    virtual void draw_line(
        const math::float3& start,
        const math::float3& end,
        const math::float3& color) override
    {
        m_drawer->draw_line(start, end, color);
    }

private:
    graphics::graphics_debug* m_drawer;
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
    m_plugin = std::make_unique<physics_plugin>();
    m_plugin->load("violet-physics-bullet3.dll");

    world_desc desc = {};
    desc.gravity = {config["gravity"][0], config["gravity"][1], config["gravity"][2]};

#if defined(VIOLET_PHYSICS_DEBUG_DRAW)
    physics_debug::instance().initialize(&system<graphics::graphics>().debug());
    m_world.reset(m_plugin->factory().make_world(desc, &physics_debug::instance()));
#else
    m_world.reset(m_plugin->factory().make_world(desc));
#endif

    auto& world = system<ecs::world>();
    world.register_component<rigidbody>();
    world.register_component<joint>();

    auto& event = system<event>();
    event.subscribe<scene::event_enter_scene>("physics", [this](ecs::entity entity) {
        m_enter_world_list.push(entity);
    });

    return true;
}

void physics::shutdown()
{
    system<event>().unsubscribe<scene::event_enter_scene>("physics");

    auto& world = system<ecs::world>();
    world.view<joint>().each([](joint& joint) { joint.reset_interface(nullptr); });
    world.view<rigidbody>().each([](rigidbody& rigidbody) { rigidbody.reset_interface(nullptr); });

    m_world = nullptr;
    m_plugin->unload();
}

void physics::simulation()
{
    auto& world = system<ecs::world>();

    system<scene::scene>().sync_local();

    while (!m_enter_world_list.empty())
    {
        initialize_entity(m_enter_world_list.front());
        m_enter_world_list.pop();
    }

    world.view<rigidbody, scene::transform>().each(
        [&](rigidbody& rigidbody, scene::transform& transform) {
            if (rigidbody.type() == rigidbody_type::KINEMATIC && transform.sync_count() != 0)
            {
                math::float4x4_simd to_world = math::simd::load(transform.to_world());
                math::float4x4_simd offset = math::simd::load(rigidbody.offset());

                math::float4x4 rigidbody_to_world;
                math::simd::store(math::matrix_simd::mul(offset, to_world), rigidbody_to_world);
                rigidbody.interface()->transform(rigidbody_to_world);
            }
        });

    m_world->simulation(system<timer>().frame_delta());

    while (true)
    {
        rigidbody_interface* updated = m_world->updated_rigidbody();
        if (updated == nullptr)
            break;

        ecs::entity entity = m_user_data[updated->user_data_index].entity;
        auto& r = world.component<rigidbody>(entity);
        auto& t = world.component<scene::transform>(entity);
        r.sync_world(updated->transform(), t);
    }

    system<scene::scene>().sync_world();

#if defined(VIOLET_PHYSICS_DEBUG_DRAW)
    m_world->debug();
#endif
}

std::unique_ptr<collision_shape_interface> physics::make_shape(const collision_shape_desc& desc)
{
    return std::unique_ptr<collision_shape_interface>(
        m_plugin->factory().make_collision_shape(desc));
}

std::unique_ptr<collision_shape_interface> physics::make_shape(
    const collision_shape_interface* const* child,
    const math::float4x4* offset,
    std::size_t size)
{
    return std::unique_ptr<collision_shape_interface>(
        m_plugin->factory().make_collision_shape(child, offset, size));
}

void physics::initialize_entity(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& relation = system<relation>();

    auto init_rigidbody = [&, this](ecs::entity node) {
        if (!world.has_component<rigidbody>(node) || !world.has_component<scene::transform>(node))
            return;

        auto& r = world.component<rigidbody>(node);
        auto& transform = world.component<scene::transform>(node);

        if (r.interface() == nullptr)
        {
            rigidbody_desc desc = {};
            desc.type = r.type();
            desc.mass = r.mass();
            desc.linear_damping = r.linear_damping();
            desc.angular_damping = r.angular_damping();
            desc.restitution = r.restitution();
            desc.friction = r.friction();
            desc.shape = r.shape();
            math::float4x4_simd to_world = math::simd::load(transform.to_world());
            math::float4x4_simd offset = math::simd::load(r.offset());
            math::simd::store(math::matrix_simd::mul(offset, to_world), desc.initial_transform);

            r.reset_interface(m_plugin->factory().make_rigidbody(desc));
            r.interface()->user_data_index = m_user_data.size();
            m_user_data.push_back({node});
        }

        m_world->add(r.interface(), r.collision_group(), r.collision_mask());
    };

    auto init_joint = [&, this](ecs::entity node) {
        if (!world.has_component<joint>(node))
            return;

        auto& j = world.component<joint>(node);
        if (j.interface() == nullptr)
        {
            joint_desc desc = {};
            desc.relative_position_a = j.relative_position_a();
            desc.relative_rotation_a = j.relative_rotation_a();
            desc.relative_position_b = j.relative_position_b();
            desc.relative_rotation_b = j.relative_rotation_b();
            desc.min_linear = j.min_linear();
            desc.max_linear = j.max_linear();
            desc.min_angular = j.min_angular();
            desc.max_angular = j.max_angular();

            for (std::size_t i = 0; i < 6; ++i)
            {
                desc.spring_enable[i] = j.spring_enable()[i];
                desc.stiffness[i] = j.stiffness()[i];
            }

            desc.rigidbody_a = world.component<rigidbody>(j.relative_a()).interface();
            desc.rigidbody_b = world.component<rigidbody>(j.relative_b()).interface();

            j.reset_interface(m_plugin->factory().make_joint(desc));
        }

        m_world->add(j.interface());
    };

    relation.each_bfs(entity, init_rigidbody);
    relation.each_bfs(entity, init_joint);
}
} // namespace violet::physics