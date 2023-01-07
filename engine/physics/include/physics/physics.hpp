#pragma once

#include "core/context.hpp"
#include "ecs/world.hpp"
#include "physics/joint.hpp"
#include "physics/physics_plugin.hpp"
#include "physics/rigidbody.hpp"
#include "scene/transform.hpp"

namespace violet::physics
{
struct rigidbody_user_data
{
    ecs::entity entity;
};

class physics : public core::system_base
{
public:
    physics() noexcept;
    virtual ~physics();

    virtual bool initialize(const dictionary& config) override;
    virtual void shutdown() override;

    std::unique_ptr<collision_shape_interface> make_shape(const collision_shape_desc& desc)
    {
        return std::unique_ptr<collision_shape_interface>(
            m_plugin.factory().make_collision_shape(desc));
    }

    std::unique_ptr<collision_shape_interface> make_shape(
        const collision_shape_interface* const* child,
        const math::float4x4* offset,
        std::size_t size)
    {
        return std::unique_ptr<collision_shape_interface>(
            m_plugin.factory().make_collision_shape(child, offset, size));
    }

    void simulation();

private:
    void initialize_entity(ecs::entity entity);

    std::unique_ptr<world_interface> m_world;

    std::vector<rigidbody_user_data> m_user_data;

    std::queue<ecs::entity> m_enter_world_list;
    std::queue<ecs::entity> m_exit_world_list;

    physics_plugin m_plugin;
};
} // namespace violet::physics