#pragma once

#include "math.hpp"
#include "scene_exports.hpp"
#include <vector>

namespace ash::scene
{
class SCENE_API scene_node
{
public:
    scene_node();

    void parent(scene_node* parent);
    scene_node* parent() const noexcept { return m_parent; }

    const std::vector<scene_node*>& children() const noexcept { return m_children; }

    math::float4x4 to_world;
    math::float4x4 to_parent;

    bool dirty;
    bool updated;
    bool in_scene;
    bool in_view;

private:
    void add_child(scene_node* child);
    void remove_child(scene_node* child);

    scene_node* m_parent;
    std::vector<scene_node*> m_children;
};
} // namespace ash::scene