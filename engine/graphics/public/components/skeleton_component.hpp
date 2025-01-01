#pragma once

#include "ecs/entity.hpp"
#include "math/types.hpp"
#include <vector>

namespace violet
{
struct skeleton_bone
{
    entity entity;
    mat4f binding_pose_inv;
};

struct skeleton_component
{
    std::vector<skeleton_bone> bones;
};
} // namespace violet