#pragma once

#include "btBulletDynamicsCommon.h"
#include "physics/physics_interface.hpp"

namespace violet::bt3
{
inline btVector3 convert_vector(const vec3f& vec)
{
    return btVector3(vec[0], vec[1], vec[2]);
}

inline vec3f convert_vector(const btVector3& vec)
{
    return vec3f{vec.x(), vec.y(), vec.z()};
}

inline btQuaternion convert_quaternion(const vec4f& q)
{
    return btQuaternion(q[0], q[1], q[2], q[3]);
}
} // namespace violet::bt3