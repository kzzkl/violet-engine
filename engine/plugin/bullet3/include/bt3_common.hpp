#pragma once

#include "btBulletDynamicsCommon.h"
#include "physics_interface.hpp"
#include <memory>

namespace ash::physics::bullet3
{
inline btVector3 convert_vector(const math::float3& vec)
{
    return btVector3(vec[0], vec[1], vec[2]);
}

inline math::float3 convert_vector(const btVector3& vec)
{
    return math::float3{vec.x(), vec.y(), vec.z()};
}

inline btQuaternion convert_quaternion(const math::float4& q)
{
    return btQuaternion(q[0], q[1], q[2], q[3]);
}
} // namespace ash::physics::bullet3