#pragma once

#include "components/collider_component.hpp"
#include "core/engine.hpp"
#include "physics/physics_context.hpp"
#include "physics/physics_scene.hpp"

namespace violet
{
#ifdef VIOLET_PHYSICS_DEBUG_DRAW
class physics_debug;
#endif

class physics_plugin;
class physics_system : public system
{
public:
    physics_system();
    virtual ~physics_system();

    bool initialize(const dictionary& config) override;

    physics_context* get_context() const noexcept
    {
        return m_context.get();
    }

private:
    void simulation();

    void update_rigidbody();
    void update_joint();
    void update_transform(entity e, const mat4f& parent_world, bool parent_dirty);

    physics_scene* get_scene(std::uint32_t layer);

    phy_collision_shape* get_shape(const std::vector<collider_shape>& shapes);

    std::vector<std::unique_ptr<physics_scene>> m_scenes;
    std::unique_ptr<physics_plugin> m_plugin;
    std::unique_ptr<physics_context> m_context;

    std::unordered_map<std::uint64_t, phy_ptr<phy_collision_shape>> m_shapes;

    std::uint32_t m_system_version{0};

    float m_time{0.0f};

#ifdef VIOLET_PHYSICS_DEBUG_DRAW
    std::unique_ptr<physics_debug> m_debug;
#endif
};
} // namespace violet