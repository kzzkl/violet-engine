#pragma once

#include "physics/physics_interface.hpp"
#include <memory>
#include <vector>

namespace violet
{
class pei_deleter
{
public:
    pei_deleter();
    pei_deleter(pei_plugin* pei);

    void operator()(pei_world* world);
    void operator()(pei_collision_shape* collision_shape);
    void operator()(pei_rigidbody* rigidbody);
    void operator()(pei_joint* joint);

private:
    pei_plugin* m_pei;
};

template <typename T>
using pei_ptr = std::unique_ptr<T, pei_deleter>;

class physics_context
{
public:
    physics_context(pei_plugin* pei);

    pei_ptr<pei_world> create_world(const pei_world_desc& desc);

    pei_ptr<pei_collision_shape> create_collision_shape(const pei_collision_shape_desc& desc);
    pei_ptr<pei_collision_shape> create_collision_shape(
        const std::vector<pei_collision_shape*>& shapes,
        const std::vector<float4x4>& offset);

    pei_ptr<pei_rigidbody> create_rigidbody(const pei_rigidbody_desc& desc);

    pei_ptr<pei_joint> create_joint(const pei_joint_desc& desc);

private:
    pei_plugin* m_pei;
    pei_deleter m_deleter;
};
} // namespace violet