#include "physics/physics_system.hpp"
#include "components/rigidbody.hpp"
#include "components/transform.hpp"
#include "physics_plugin.hpp"

namespace violet
{
class rigidbody_component_info : public component_info_default<rigidbody>
{
public:
    rigidbody_component_info(physics_context* context) : m_context(context) {}

    virtual void construct(actor* owner, void* target) override
    {
        new (target) rigidbody(m_context);
    }

private:
    physics_context* m_context;
};

physics_system::physics_system() : engine_system("physics")
{
}

physics_system::~physics_system()
{
}

bool physics_system::initialize(const dictionary& config)
{
    m_plugin = std::make_unique<physics_plugin>();
    m_plugin->load(config["plugin"]);
    m_context = std::make_unique<physics_context>(m_plugin->get_pei());

    if (config["tick"])
    {
        on_frame_begin().then(
            [this]()
            {
                simulation();
            });
    }

    get_world().register_component<rigidbody, rigidbody_component_info>(m_context.get());

    return true;
}

void physics_system::shutdown()
{
}

void physics_system::simulation(physics_world* world, bool immediately)
{
    if (world != nullptr && !immediately)
    {
        m_worlds.push_back(world);
        return;
    }

    view<transform, rigidbody> view(get_world());
    view.each(
        [](transform& transform, rigidbody& rigidbody)
        {
            if (rigidbody.get_type() == PEI_RIGIDBODY_TYPE_KINEMATIC)
            {
                float4x4_simd world_matrix = simd::load(transform.get_world_matrix());
                float4x4_simd offset_matrix = simd::load(rigidbody.get_offset());
                world_matrix = matrix_simd::mul(offset_matrix, world_matrix);

                float4x4 world;
                simd::store(world_matrix, world);
                rigidbody.get_rigidbody()->set_transform(world);
            }
        });

    float step = get_timer().get_frame_delta();

    if (world == nullptr)
    {
        for (physics_world* world : m_worlds)
            world->simulation(step);

        m_worlds.clear();
    }
    else
    {
        world->simulation(step);
    }

    struct updated_object
    {
        transform* transform;
        rigidbody* rigidbody;
        std::size_t depth;
    };
    std::vector<updated_object> updated_objects;

    view.each(
        [&updated_objects](transform& transform, rigidbody& rigidbody)
        {
            if (!rigidbody.get_updated_flag())
                return;

            rigidbody.set_updated_flag(false);

            std::size_t depth = 0;
            auto parent = transform.get_parent();
            while (parent)
            {
                ++depth;
                parent = parent->get_parent();
            }

            updated_objects.push_back(updated_object{&transform, &rigidbody, depth});
        });

    std::sort(
        updated_objects.begin(),
        updated_objects.end(),
        [](const updated_object& a, const updated_object& b)
        {
            return a.depth < b.depth;
        });

    for (updated_object& object : updated_objects)
    {
        float4x4_simd world_matrix = simd::load(object.rigidbody->get_transform());
        float4x4_simd offset_matrix = simd::load(object.rigidbody->get_offset_inverse());
        world_matrix = matrix_simd::mul(offset_matrix, world_matrix);

        float4x4 world;
        simd::store(world_matrix, world);
        world =
            object.rigidbody->get_reflector()->reflect(world, object.transform->get_world_matrix());
        object.transform->set_world_matrix(world);
    }
}
} // namespace violet