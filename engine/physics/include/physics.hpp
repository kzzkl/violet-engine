#pragma once

#include "context.hpp"
#include "joint.hpp"
#include "physics_plugin.hpp"
#include "rigidbody.hpp"
#include "transform.hpp"
#include "view.hpp"

namespace ash::physics
{
struct rigidbody_user_data
{
    ecs::entity entity;
};

class physics : public ash::core::system_base
{
public:
    static constexpr const char* TASK_SIMULATION = "physics simulation";

public:
    physics() noexcept;
    virtual ~physics();

    virtual bool initialize(const dictionary& config) override;

    std::unique_ptr<collision_shape_interface> make_shape(const collision_shape_desc& desc)
    {
        return std::unique_ptr<collision_shape_interface>(m_factory->make_collision_shape(desc));
    }

    std::unique_ptr<collision_shape_interface> make_shape(
        const collision_shape_interface* const* child,
        const math::float4x4* offset,
        std::size_t size)
    {
        return std::unique_ptr<collision_shape_interface>(
            m_factory->make_collision_shape(child, offset, size));
    }

private:
    void simulation();

    ash::ecs::view<rigidbody, ash::scene::transform>* m_rigidbody_view;
    ash::ecs::view<rigidbody, joint>* m_joint_view;

    std::unique_ptr<world_interface> m_world;

    physics_plugin m_plugin;
    factory* m_factory;

    std::vector<rigidbody_user_data> m_user_data;

#if defined(ASH_PHYSICS_DEBUG_DRAW)
    class physics_debug;
    std::unique_ptr<physics_debug> m_debug;
#endif
};
} // namespace ash::physics