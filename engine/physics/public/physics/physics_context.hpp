#pragma once

#include "physics/physics_interface.hpp"
#include <memory>
#include <span>

namespace violet
{
class phy_deleter
{
public:
    phy_deleter();
    phy_deleter(phy_plugin* plugin);

    void operator()(phy_world* world);
    void operator()(phy_collision_shape* collision_shape);
    void operator()(phy_rigidbody* rigidbody);
    void operator()(phy_joint* joint);

private:
    phy_plugin* m_plugin;
};

template <typename T>
using phy_ptr = std::unique_ptr<T, phy_deleter>;

class physics_context
{
public:
    physics_context(phy_plugin* plugin);

    phy_ptr<phy_world> create_world(const phy_world_desc& desc);

    phy_ptr<phy_collision_shape> create_collision_shape(const phy_collision_shape_desc& desc);
    phy_ptr<phy_collision_shape> create_collision_shape(
        std::span<phy_collision_shape*> shapes,
        std::span<mat4f> offset);

    phy_ptr<phy_rigidbody> create_rigidbody(const phy_rigidbody_desc& desc);

    phy_ptr<phy_joint> create_joint(const phy_joint_desc& desc);

private:
    phy_plugin* m_plugin;
    phy_deleter m_deleter;
};
} // namespace violet