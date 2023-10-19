#include "physics/physics_system.hpp"
#include "components/transform.hpp"
#include "core/engine.hpp"
#include "physics_plugin.hpp"

#if defined(VIOLET_PHYSICS_DEBUG_DRAW)
#    include "graphics/graphics.hpp"
#endif

namespace violet
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

    engine::on_frame_begin().then(
        [this]()
        {
            simulation();
        });

    return true;
}

void physics_system::shutdown()
{
}

void physics_system::simulation(physics_world* world)
{
    m_worlds.push_back(world);
}

pei_plugin* physics_system::get_pei() const noexcept
{
    return m_plugin->get_pei();
}

void physics_system::simulation()
{
    float step = engine::get_timer().get_frame_delta();
    for (physics_world* world : m_worlds)
        world->simulation(step);

    struct updated_object
    {
        transform* transform;
        rigidbody* rigidbody;
        std::size_t depth;
    };
    std::vector<updated_object> updated_objects;

    view<transform, rigidbody> view(engine::get_world());
    view.each(
        [&updated_objects](transform& t, rigidbody& r)
        {
            if (!r.get_updated_flag())
                return;

            r.set_updated_flag(false);

            std::size_t depth = 0;
            auto parent = t.get_parent();
            while (parent)
            {
                ++depth;
                parent = parent->get_parent();
            }

            updated_objects.push_back(updated_object{&t, &r, depth});
        });

    std::sort(
        updated_objects.begin(),
        updated_objects.end(),
        [](const updated_object& a, const updated_object& b)
        {
            return a.depth < b.depth;
        });

    for (updated_object& object : updated_objects)
        object.transform->set_world_matrix(object.rigidbody->get_transform());

    m_worlds.clear();
}
} // namespace violet