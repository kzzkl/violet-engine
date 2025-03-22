#pragma once

#include "components/rigidbody_component.hpp"
#include "ecs/component.hpp"
#include "physics/physics_scene.hpp"

namespace violet
{
class rigidbody_motion_state : public phy_motion_state
{
public:
    const mat4f& get_transform() const override
    {
        return transform;
    }

    void set_transform(const mat4f& m) override
    {
        transform = m;
        dirty = true;
    }

    bool dirty{false};
    mat4f transform;
};

class rigidbody_motion_state_kinematic : public rigidbody_motion_state
{
public:
    void set_transform(const mat4f& transform) override {}
};

struct rigidbody_component_meta
{
    rigidbody_component_meta() = default;
    rigidbody_component_meta(const rigidbody_component_meta&) = delete;
    rigidbody_component_meta(rigidbody_component_meta&& other) noexcept
        : scene(other.scene),
          rigidbody(std::move(other.rigidbody)),
          motion_state(std::move(other.motion_state))
    {
        other.scene = nullptr;
        other.rigidbody = nullptr;
        other.motion_state = nullptr;
    }

    ~rigidbody_component_meta()
    {
        if (scene != nullptr)
        {
            scene->remove_rigidbody(rigidbody.get());
        }
    }

    rigidbody_component_meta& operator=(const rigidbody_component_meta&) = delete;
    rigidbody_component_meta& operator=(rigidbody_component_meta&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        scene = other.scene;
        rigidbody = std::move(other.rigidbody);
        motion_state = std::move(other.motion_state);
        other.scene = nullptr;
        other.rigidbody = nullptr;
        other.motion_state = nullptr;

        return *this;
    }

    physics_scene* scene{nullptr};
    phy_ptr<phy_rigidbody> rigidbody;

    std::unique_ptr<rigidbody_motion_state> motion_state;
};

template <>
struct component_trait<rigidbody_component_meta>
{
    using main_component = rigidbody_component;
};
} // namespace violet