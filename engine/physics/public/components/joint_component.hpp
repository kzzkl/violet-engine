#pragma once

#include "ecs/entity.hpp"
#include "math/types.hpp"
#include "physics/physics_scene.hpp"
#include <vector>

namespace violet
{
struct joint_meta
{
    joint_meta() = default;
    joint_meta(const joint_meta&) = delete;
    joint_meta(joint_meta&& other) noexcept
        : scene(other.scene),
          joint(std::move(other.joint))
    {
        other.scene = nullptr;
        other.joint = nullptr;
    }

    ~joint_meta()
    {
        if (scene != nullptr)
        {
            scene->remove_joint(joint.get());
        }
    }

    joint_meta& operator=(const joint_meta&) = delete;
    joint_meta& operator=(joint_meta&& other) noexcept
    {
        if (this != &other)
        {
            scene = other.scene;
            joint = std::move(other.joint);
            other.scene = nullptr;
            other.joint = nullptr;
        }

        return *this;
    }

    physics_scene* scene{nullptr};
    phy_ptr<phy_joint> joint;
};

struct joint
{
    entity target;

    vec3f source_position;
    vec4f source_rotation{0.0, 0.0, 0.0, 1.0};

    vec3f target_position;
    vec4f target_rotation{0.0, 0.0, 0.0, 1.0};

    vec3f min_linear;
    vec3f max_linear;

    vec3f min_angular;
    vec3f max_angular;

    bool spring_enable[6];
    float stiffness[6];
    float damping[6];

    joint_meta meta;
};

struct joint_component
{
    std::vector<joint> joints;
};
} // namespace violet