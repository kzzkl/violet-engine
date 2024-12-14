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
        return m_transform;
    }

    void set_transform(const mat4f& transform) override
    {
        m_transform = transform;
        dirty = true;
    }

    bool dirty{false};

private:
    mat4f m_transform;
};

struct rigidbody_meta_component
{
    rigidbody_meta_component() = default;
    rigidbody_meta_component(const rigidbody_meta_component&) = delete;
    rigidbody_meta_component(rigidbody_meta_component&& other) noexcept
        : scene(other.scene),
          rigidbody(std::move(other.rigidbody)),
          motion_state(std::move(other.motion_state))
    {
        other.scene = nullptr;
        other.rigidbody = nullptr;
        other.motion_state = nullptr;
    }

    ~rigidbody_meta_component()
    {
        if (scene != nullptr)
        {
            scene->remove_rigidbody(rigidbody.get());
        }
    }

    rigidbody_meta_component& operator=(const rigidbody_meta_component&) = delete;
    rigidbody_meta_component& operator=(rigidbody_meta_component&& other) noexcept
    {
        if (this != &other)
        {
            scene = other.scene;
            rigidbody = std::move(other.rigidbody);
            motion_state = std::move(other.motion_state);
            other.scene = nullptr;
            other.rigidbody = nullptr;
            other.motion_state = nullptr;
        }

        return *this;
    }

    physics_scene* scene{nullptr};
    phy_ptr<phy_rigidbody> rigidbody;

    std::unique_ptr<rigidbody_motion_state> motion_state;
};

template <>
struct component_trait<rigidbody_meta_component>
{
    using main_component = rigidbody_component;
};
} // namespace violet