#pragma once

#include "context.hpp"
#include "physics_exports.hpp"
#include "physics_plugin.hpp"
#include "rigidbody.hpp"
#include "transform.hpp"
#include "view.hpp"

namespace ash::physics
{
class physics_debug;
class PHYSICS_API physics : public ash::core::submodule
{
public:
    static constexpr uuid id = "d409fbd0-a274-44c9-b760-a2476794d383";
    static constexpr const char* TASK_SIMULATION = "physics simulation";

public:
    physics() noexcept;
    virtual ~physics();

    virtual bool initialize(const dictionary& config) override;

    std::unique_ptr<collision_shape_interface> make_shape()
    {
        collision_shape_desc desc;
        desc.type = collision_shape_type::BOX;
        desc.box.length = 1.0f;
        desc.box.width = 1.0f;
        desc.box.height = 1.0f;
        return std::unique_ptr<collision_shape_interface>(m_factory->make_collision_shape(desc));
    }

    std::unique_ptr<rigidbody_interface> make_rigidbody(
        collision_shape_interface* shape,
        const math::float4x4& world_matrix)
    {
        rigidbody_desc desc;
        desc.mass = 1.0f;
        desc.shape = shape;
        desc.world_matrix = world_matrix;
        return std::unique_ptr<rigidbody_interface>(m_factory->make_rigidbody(desc));
    }

private:
    void simulation();

    ash::ecs::view<rigidbody, ash::scene::transform>* m_view;

    std::unique_ptr<world_interface> m_world;
    std::unique_ptr<physics_debug> m_debug;

    physics_plugin m_plugin;
    factory* m_factory;
};
} // namespace ash::physics