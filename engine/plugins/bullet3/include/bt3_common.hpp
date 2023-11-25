#pragma once

#include "btBulletDynamicsCommon.h"
#include "physics/physics_interface.hpp"

namespace violet::bt3
{
inline btVector3 convert_vector(const float3& vec)
{
    return btVector3(vec[0], vec[1], vec[2]);
}

inline float3 convert_vector(const btVector3& vec)
{
    return float3{vec.x(), vec.y(), vec.z()};
}

inline btQuaternion convert_quaternion(const float4& q)
{
    return btQuaternion(q[0], q[1], q[2], q[3]);
}
} // namespace violet::bt3